#!/usr/bin/env python3
"""lanes/t-slice1/check_driver_cases.py: an independent check of the hand-written expected lines of
tests/driver/i-*.out.

The expected lines were written by hand from the mathematics (docs/SPEC.md 5, docs/conventions.md 5.6, 5.7, 9.3 to
9.5, docs/proofs/ideles.md), before the driver existed.  This script re-derives every line with exact integers and
exact rationals (fractions.Fraction, math.gcd, pow(x, -1, m)) and the reference reader and printer
proto/text_grammar.py (canonical text of a value, print_real of 9.5).  It shares no code with the C library or with
the driver.  Where a line is a status, it is derived from the reference reader (PARSE, DOMAIN, LIMIT of a value
text), from the rules of the mathematics (NOT_UNIT for the exact zero, UNIT_NOT_CERTIFIED for a ball of positive
radius) or, for NOT_DETERMINED, from the arithmetic of the ends of the interval (the check says which facts it
verifies).  The pair rules of the driver (which pairs of types an operation accepts) are the table PAIRS below:
that table is a design decision of the lane, not mathematics.

    python3 lanes/t-slice1/check_driver_cases.py          (from the repository root)

Exit status 0 if every line of every file agrees, 1 otherwise; the disagreeing lines are printed.
"""
import glob
import math
import os
import sys
from fractions import Fraction

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "proto"))
import text_grammar as tg  # noqa: E402

SEP = " with "
WORD_MAX = 2 ** 63 - 1


class Err(Exception):
    def __init__(self, st):
        Exception.__init__(self, st)
        self.st = st


# ---------------------------------------------------------------------------------------------- unit cosets

def unorm(u):
    return tg.ucoset_normal(u)


def ufmt(u):
    c, N = unorm(u)
    return "[%d]" % c if N == 0 else "[%d mod %d]" % (c, N)


def gcd0(a, b):
    return math.gcd(a, b)          # gcd(0, n) = n, gcd(0, 0) = 0


def ured(c, N):
    if N == 0:
        return (c, 0)
    c %= N
    return (c if c else N, N)


def umul(u, v):
    g = gcd0(u[1], v[1])
    if g == 0:
        return (u[0] * v[0], 0)
    return ured(u[0] * v[0], g)


def uinv(u):
    c, N = unorm(u)
    if N == 0:
        return (c, 0)
    if N == 1:
        return (1, 1)
    return ured(pow(c, -1, N), N)


def upow(u, k):
    c, N = unorm(u)
    if k == 0:
        return (1, 0)
    if N == 0:
        return (c ** abs(k), 0)          # c = +-1
    if N == 1:
        return (1, 1)
    return ured(pow(c, k, N), N)


def factor(n):
    f = {}
    p = 2
    while p * p <= n:
        while n % p == 0:
            f[p] = f.get(p, 0) + 1
            n //= p
        p += 1
    if n > 1:
        f[n] = f.get(n, 0) + 1
    return f


def is_prime(p):
    return p > 1 and all(p % q for q in range(2, int(p ** 0.5) + 1))


def upow_tight(u, k):
    """docs/proofs/ideles.md Proposition 13 as stated in include/adelefeld/idpow.h (M_k = A B)."""
    c, N = unorm(u)
    if k == 0:
        return (1, 0)
    if N == 0:
        return upow(u, k)
    K = abs(k)
    fk = factor(K)
    s = 1
    for p in factor(N) if N > 1 else {}:
        s *= p ** fk.get(p, 0)
    A = N * s
    B = 1
    if N % 2 == 1 and K % 2 == 0:
        B *= 2 ** (2 + fk.get(2, 0))
    for d in range(1, K + 1):
        if K % d == 0:
            p = d + 1
            if p > 2 and is_prime(p) and N % p != 0:
                B *= p ** (1 + fk.get(p, 0))
    M = A * B
    ck = pow(c, k, A) if A > 1 else 0
    # x = c^k mod A, x = 1 mod B (CRT)
    x = ck
    if B > 1:
        x = (ck + A * ((1 - ck) * pow(A, -1, B) % B)) % M
    return ured(x, M) if M > 1 else (1, 1)


# ---------------------------------------------------------------------------------------------- reals

def fmt_rat(x):
    return tg.fmt_rat(Fraction(x))


def real_text(mid, rad, digits, cond):
    return tg.print_real(Fraction(mid), Fraction(rad), digits, cond)


# ---------------------------------------------------------------------------------------------- values

class V:
    def __init__(self, kind, **kw):
        self.kind = kind
        self.__dict__.update(kw)


def parse_value(text, digits):
    """The value of an operand text with the reference reader; raises Err(status)."""
    data = text.encode("ascii")
    cls = tg.classify(data)
    if cls.startswith("!"):
        raise Err(cls[1:])
    if cls == "rat":
        out = tg.canonical("rat", data)
        if out.startswith("!"):
            raise Err(out[1:])
        s = tg._prep(data, tg.DEFAULT_LIMITS)
        return V("rat", q=tg._rat(tg._syntax("rat", s)[1]))
    if cls in ("ucoset", "idele", "idclass", "adele", "fball", "cadele"):
        out = tg.canonical(cls, data)
        if out.startswith("!"):
            raise Err(out[1:])
        s = tg._prep(data, tg.DEFAULT_LIMITS)
        node = tg._syntax(cls, s)
        if cls == "ucoset":
            return V("ucoset", u=tg._ucoset(node[1]))
        if cls == "idele":
            mid, rad = tg._real(node[1])
            return V("idele", mid=mid, rad=rad, r=tg._rat(node[2]), u=tg._ucoset(node[3]))
        if cls == "idclass":
            mid, rad = tg._real(node[1])
            return V("idclass", mid=mid, rad=rad, u=tg._ucoset(node[2]))
        if cls == "adele":
            (mid, rad), (a, N) = tg._adele(node)
            return V("adele", mid=mid, rad=rad, a=a, N=N)
        return V(cls)
    return V("other")


def show_value(v, digits):
    if v.kind == "rat":
        return fmt_rat(v.q)
    if v.kind == "ucoset":
        return ufmt(v.u)
    if v.kind == "idele":
        return "(%s ; %s * %s)" % (real_text(v.mid, v.rad, digits, "nonzero"), fmt_rat(v.r), ufmt(v.u))
    if v.kind == "idclass":
        return "<%s ; %s>" % (real_text(v.mid, v.rad, digits, "positive"), ufmt(v.u))
    raise AssertionError(v.kind)


def fin_text(a, N):
    a, N = Fraction(a), Fraction(N)
    if N > 0:
        a = a - N * math.floor(a / N)
        return "%s mod %s" % (fmt_rat(a), fmt_rat(N))
    return fmt_rat(a)


def exact(v):
    assert v.rad == 0, "an operand with a radius in a case that needs an exact real part"
    return v.mid


def lcm(a, b):
    return a * b // math.gcd(a, b)


def odd_rep(c, N):
    return c if c % 2 == 1 else c + N


# the pair rules of the driver: operation -> the accepted (kind, kind) pairs (a design decision of the lane)
PAIRS = {
    "mul": {("ucoset", "ucoset"), ("idele", "idele"), ("idclass", "idclass"), ("idele", "rat"), ("rat", "idele")},
    "div": {("ucoset", "ucoset"), ("idele", "idele"), ("idclass", "idclass"), ("idele", "rat"), ("adele", "idele")},
}
UNARY = {
    "neg": {"ucoset", "idele"},
    "inv": {"ucoset", "idele", "idclass", "rat"},
    "class": {"idele"},
    "norm": {"idele", "idclass"},
    "hull": {"idele"},
    "hullsimple": {"idele"},
    "unitof": {"adele"},
}
POWPAIRS = {"ucoset", "idele", "idclass"}


def int_operand(v):
    if v.kind != "rat" or v.q.denominator != 1:
        raise Err("DOMAIN")
    if abs(v.q.numerator) > WORD_MAX + (1 if v.q.numerator < 0 else 0):
        raise Err("LIMIT")
    return v.q.numerator


def place_operand(tok):
    if tok == "real":
        return "real"
    if not tok.isdigit() or (len(tok) > 1 and tok[0] == "0"):
        raise Err("PARSE")
    p = int(tok)
    if p < 2 or p >= 2 ** 64 or not is_prime(p):
        raise Err("DOMAIN")
    return p


def vp(r, p):
    n, d = r.numerator, r.denominator
    v = 0
    while n % p == 0:
        n //= p
        v += 1
    while d % p == 0:
        d //= p
        v -= 1
    return v


def run_command(line, st):
    """The expected output line of one command, or raises Err."""
    words = line.split(" ", 1)
    op = words[0]
    rest = words[1] if len(words) > 1 else ""
    digits, prec = st["digits"], st["prec"]
    if op in ("prec", "digits"):
        v = int(rest)
        if op == "prec":
            if v > 100000:
                raise Err("LIMIT")
            st["prec"] = v
        else:
            st["digits"] = v
        return None
    ops = rest.split(SEP) if rest else []
    arity = {"show": 1, "type": 1, "mul": 2, "div": 2, "neg": 1, "equal": 2, "contains": 2, "overlaps": 2, "add": 2,
             "sub": 2, "inv": 1, "pow": 2, "powtight": 2, "norm": 1, "class": 1, "idele": 1, "hull": 1,
             "hullsimple": 1, "unitof": 1, "valuation": 2, "abs": 2, "dump": 1}[op]
    if len(ops) != arity:
        raise Err("PARSE")
    if op == "type":
        c = tg.classify(ops[0].encode("ascii"))
        if c.startswith("!"):
            raise Err(c[1:])
        return c
    if op in ("valuation", "abs"):
        x = parse_value(ops[0], digits)
        pl = ops[1]
        if " " in pl:
            raise Err("PARSE")
        place = place_operand(pl)
        if x.kind != "idele":
            raise Err("DOMAIN")
        if place == "real":
            if op == "valuation":
                raise Err("DOMAIN")
            return real_text(abs(x.mid), x.rad, digits, None)
        v = vp(x.r, place)
        if op == "valuation":
            return str(v)
        return fmt_rat(Fraction(place) ** (-v))
    vals = [parse_value(t, digits) for t in ops]
    kinds = tuple(v.kind for v in vals)
    if op == "dump":
        raise Err("UNSUPPORTED")
    if op == "show":
        return show_value(vals[0], digits)
    if op in ("add", "sub"):
        raise Err("DOMAIN")
    if op in ("equal", "contains", "overlaps"):
        if kinds != ("ucoset", "ucoset"):
            raise Err("DOMAIN")
        a, b = unorm(vals[0].u), unorm(vals[1].u)
        if op == "equal":
            return "true" if a == b else "false"
        if op == "contains":
            (c, N), (c2, N2) = a, b
            if N2 == 0:
                return "true" if (N == 0 and c == c2) else "false"
            if N == 0:
                return "true" if (c - c2) % N2 == 0 else "false"
            return "true" if (N % N2 == 0 and (c - c2) % N2 == 0) else "false"
        (c, N), (c2, N2) = a, b
        g = gcd0(N, N2)
        return "true" if ((c == c2) if g == 0 else (c - c2) % g == 0) else "false"
    if op in ("pow", "powtight"):
        if kinds[0] not in POWPAIRS:
            raise Err("DOMAIN")
        k = int_operand(vals[1])
        x = vals[0]
        f = upow if op == "pow" else upow_tight
        if x.kind == "ucoset":
            return ufmt(f(x.u, k))
        m = Fraction(1) if k == 0 else exact(x) ** k       # x^0 = 1, exact, whatever the ball
        if x.kind == "idele":
            return show_value(V("idele", mid=m, rad=Fraction(0), r=x.r ** k, u=f(x.u, k)), digits)
        return show_value(V("idclass", mid=m, rad=Fraction(0), u=f(x.u, k)), digits)
    if op in PAIRS:
        if kinds not in PAIRS[op]:
            raise Err("DOMAIN")
        return binary(op, vals, digits)
    if op in UNARY or op == "idele":
        return unary(op, vals[0], digits)
    raise AssertionError(op)


def binary(op, vals, digits):
    x, y = vals
    k = (x.kind, y.kind)
    if k == ("ucoset", "ucoset"):
        return ufmt(umul(x.u, y.u if op == "mul" else uinv(y.u)))
    if k == ("idele", "idele"):
        if op == "div":
            y = V("idele", mid=1 / exact(y), rad=Fraction(0), r=1 / y.r, u=uinv(y.u))
        return show_value(V("idele", mid=exact(x) * exact(y), rad=Fraction(0), r=x.r * y.r, u=umul(x.u, y.u)),
                          digits)
    if k == ("idclass", "idclass"):
        if op == "div":
            y = V("idclass", mid=1 / exact(y), rad=Fraction(0), u=uinv(y.u))
        return show_value(V("idclass", mid=exact(x) * exact(y), rad=Fraction(0), u=umul(x.u, y.u)), digits)
    if k in (("idele", "rat"), ("rat", "idele")):
        i, q = (x, y.q) if k[0] == "idele" else (y, x.q)
        if op == "div":
            q = 1 / q if q != 0 else None
        if q is None or q == 0:
            raise Err("NOT_UNIT")
        sg = -1 if q < 0 else 1
        return show_value(V("idele", mid=exact(i) * q, rad=Fraction(0), r=i.r * abs(q), u=umul(i.u, (sg, 0))), digits)
    if k == ("adele", "idele"):
        a, M = x.a, x.N
        assert a.denominator == 1 and M.denominator == 1
        a, M = int(a), int(M)
        c, N = unorm(y.u)
        r = y.r
        if N == 0:
            fa, fM = Fraction(a * c) / r, Fraction(M) / r
        else:
            e = odd_rep(pow(c, -1, N), N) if N > 1 else 1
            fa, fM = Fraction(a * e) / r, Fraction(gcd0(abs(a) * lcm(N, 2), M)) / r
        return "(%s ; %s)" % (real_text(exact(x) / exact(y), Fraction(0), digits, None), fin_text(fa, fM))
    raise AssertionError(k)


def unary(op, x, digits):
    if op == "idele":
        if x.kind != "rat":
            raise Err("DOMAIN")
        q = x.q
        if q == 0:
            raise Err("NOT_UNIT")
        return show_value(V("idele", mid=q, rad=Fraction(0), r=abs(q), u=(1 if q > 0 else -1, 0)), digits)
    if x.kind not in UNARY[op]:
        raise Err("DOMAIN")
    if op == "neg":
        if x.kind == "ucoset":
            return ufmt(umul(x.u, (-1, 0)))
        return show_value(V("idele", mid=-x.mid, rad=x.rad, r=x.r, u=umul(x.u, (-1, 0))), digits)
    if op == "inv":
        if x.kind == "rat":
            if x.q == 0:
                raise Err("NOT_UNIT")
            return fmt_rat(1 / x.q)
        if x.kind == "ucoset":
            return ufmt(uinv(x.u))
        if x.kind == "idele":
            return show_value(V("idele", mid=1 / exact(x), rad=Fraction(0), r=1 / x.r, u=uinv(x.u)), digits)
        return show_value(V("idclass", mid=1 / exact(x), rad=Fraction(0), u=uinv(x.u)), digits)
    if op == "class":
        sg = -1 if x.mid < 0 else 1
        return show_value(V("idclass", mid=abs(x.mid) / x.r, rad=x.rad / x.r, u=umul(x.u, (sg, 0))), digits)
    if op == "norm":
        if x.kind == "idclass":
            return real_text(x.mid, x.rad, digits, None)
        return real_text(abs(x.mid) / x.r, x.rad / x.r, digits, None)
    if op in ("hull", "hullsimple"):
        c, N = unorm(x.u)
        r = x.r
        if N == 0:
            a, m = r * c, Fraction(0)
        elif op == "hull":
            a, m = r * odd_rep(c, N), r * lcm(N, 2)
        else:
            a, m = r * c, r * N
        return "(%s ; %s)" % (real_text(x.mid, x.rad, digits, None), fin_text(a, m))
    if op == "unitof":
        if x.a == 0 and x.N == 0 or (x.mid == 0 and x.rad == 0):
            raise Err("NOT_UNIT")
        if x.N != 0 or (x.mid - x.rad <= 0 <= x.mid + x.rad):
            raise Err("UNIT_NOT_CERTIFIED")
        return show_value(V("idele", mid=x.mid, rad=x.rad, r=abs(x.a), u=(1 if x.a > 0 else -1, 0)), digits)
    raise AssertionError(op)


# ---------------------------------------------------------------------------------------------- NOT_DETERMINED

def ub30(r):
    """The least dyadic number with an odd mantissa of at most 30 bits that is >= r > 0 (mag rounding up)."""
    r = Fraction(r)
    e = r.numerator.bit_length() - r.denominator.bit_length() - 29
    while True:
        t = -((-r) // Fraction(2) ** e)          # ceil(r / 2^e)
        if t <= 2 ** 30:
            return Fraction(t) * Fraction(2) ** e
        e += 1


def e_of(t):
    t = Fraction(t)
    e0 = t.numerator.bit_length() - t.denominator.bit_length()
    return (e0 if t >= Fraction(2) ** e0 else e0 - 1) + 1


def rd_ru(t, p):
    """RD_p(t) and RU_p(t) for a rational t > 0: the round down and up to p bits."""
    t = Fraction(t)
    e = e_of(t) - p
    lo = (t // Fraction(2) ** e) * Fraction(2) ** e
    hi = lo if lo == t else lo + Fraction(2) ** e
    return lo, hi


def check_not_determined(line, prec):
    """The facts behind an expected NOT_DETERMINED of a show (the reader): the enclosing ball contains 0 (the
    radius rounded up to 30 bits reaches the midpoint) and the end points of the exact interval, rounded outwards,
    are more than prec binades apart.  Returns a string that names a failed fact, or None."""
    op, rest = line.split(" ", 1)
    assert op == "show"
    data = rest.encode("ascii")
    cls = tg.classify(data)
    s = tg._prep(data, tg.DEFAULT_LIMITS)
    node = tg._syntax(cls, s)
    mid, rad = tg._real(node[1])
    lo, hi = mid - rad, mid + rad
    a, b = (lo, hi) if lo > 0 else (-hi, -lo)
    assert a > 0
    # the enclosing ball: midpoint mid (exact here), radius ub30(rad): it contains 0 iff ub30(rad) >= |mid|
    if not (ub30(rad) >= abs(mid)):
        return "the enclosing ball does not contain 0"
    p = max(prec, 2)
    _, hu = rd_ru(b, p)
    ld, _ = rd_ru(a, p)
    if not (e_of(hu) - e_of(ld) > p):
        return "e(hi') - e(lo') = %d is not above p = %d" % (e_of(hu) - e_of(ld), p)
    return None


def check_not_determined_square(line, prec):
    """mul or pow 2 of the ball 1 +/- 0.9999999999999 read at prec: the ends of the square are more than prec
    binades apart, whatever the rounding of the ends."""
    r = Fraction(9999999999999, 10 ** 13)
    p = max(prec, 2)
    ld, _ = rd_ru(1 - r, p)                # the lower end of the ball is exactly lo' (kernel B4)
    _, hu = rd_ru(1 + r, p)
    # the reading succeeds: e(hi') - e(lo') <= p
    if not (e_of(hu) - e_of(ld) <= p):
        return "the reading does not succeed at p = %d" % p
    # the upper end of the ball is at most hi' (1 + 2^-29); the square of the lower end is exact
    lo2 = ld * ld
    hi2 = hu * hu
    if not (e_of(hi2) - e_of(lo2) > p):
        return "the gap of the squares is %d, not above p = %d" % (e_of(hi2) - e_of(lo2), p)
    return None


# ---------------------------------------------------------------------------------------------- the files

def main():
    bad = 0
    total = 0
    files = sorted(glob.glob(os.path.join(ROOT, "tests", "driver", "i-*.cmd")))
    for cmd in files:
        name = os.path.basename(cmd)
        want = [ln.rstrip("\n") for ln in open(cmd[:-4] + ".out", encoding="ascii")]
        st = {"digits": 20, "prec": 64}
        idx = 0
        for no, ln in enumerate(open(cmd, encoding="ascii"), 1):
            ln = ln.rstrip("\n")
            if not ln.strip() or ln.lstrip().startswith("#"):
                continue
            word = ln.split(" ", 1)[0]
            if word in ("prec", "digits"):
                try:
                    out = run_command(ln, st)
                except Err as e:
                    out = "error: " + e.st
                if out is None:
                    continue
            else:
                w = want[idx] if idx < len(want) else None
                if w == "error: NOT_DETERMINED":
                    # not derived by exact arithmetic alone: the facts that force it are checked
                    if word == "show":
                        why = check_not_determined(ln, st["prec"])
                    else:
                        why = check_not_determined_square(ln, st["prec"])
                    if why is not None:
                        print("%s:%d: NOT_DETERMINED is not forced: %s" % (name, no, why))
                        bad += 1
                    out = w
                else:
                    try:
                        out = run_command(ln, st)
                    except Err as e:
                        out = "error: " + e.st
            w = want[idx] if idx < len(want) else None
            idx += 1
            total += 1
            if out != w:
                print("%s:%d: %s\n    derived: %s\n    file:    %s" % (name, no, ln, out, w))
                bad += 1
        if idx != len(want):
            print("%s: %d commands with an output, %d expected lines" % (name, idx, len(want)))
            bad += 1
    print("check_driver_cases: %d lines in %d files, %d disagreements" % (total, len(files), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
