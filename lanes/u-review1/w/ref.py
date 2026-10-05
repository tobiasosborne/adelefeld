"""My own reference for the five bodies (conventions 10.1, 10.2, 5.6-5.9, 8.2, 8.4, 8.5).
Written from the conventions, not from proto/text_grammar.py.  Exact integers."""
import re
from math import gcd

OK, DOMAIN, UNSUPPORTED, PARSE, LIMIT = 0, 7, 8, 9, 10
WORD = 1 << 64
SLMAX = (1 << 63) - 1
SLMIN = -(1 << 63)
DEF = (1048576, 100000, 100000, 1048576)

HRE = re.compile(r"\A(0|-?[1-9a-f][0-9a-f]*)\Z")


class Fail(Exception):
    def __init__(self, st):
        self.st = st


def is_prime(n):
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


class T:
    def __init__(self, toks):
        self.t = toks
        self.i = 0

    def left(self):
        return len(self.t) - self.i

    def raw(self):
        if self.i >= len(self.t):
            raise Fail(PARSE)
        x = self.t[self.i]
        self.i += 1
        return x

    def h(self):
        x = self.raw()
        if not HRE.match(x):
            raise Fail(PARSE)
        return int(x, 16)

    def kw(self, *ws):
        x = self.raw()
        if x not in ws:
            raise Fail(PARSE)
        return x

    def count(self):
        n = self.h()
        if n < 0 or n > self.left():
            raise Fail(PARSE)
        return n

    def arb(self):
        return tuple(self.h() for _ in range(4))


def arb_ok(a):
    m, e, rm, re_ = a
    if m == 0:
        if e != 0:
            return False
    elif m % 2 == 0:
        return False
    if rm == 0:
        return re_ == 0
    return rm % 2 == 1 and 0 < rm < (1 << 30)


def arb_sign(a):
    """+1 all points positive, -1 all negative, 0 contains 0 (needs arb_ok)."""
    m, e, rm, re_ = a
    if m == 0:
        return 0
    s = 1 if m > 0 else -1
    m = abs(m)
    if rm == 0:
        return s
    l1 = e + m.bit_length()
    l2 = re_ + rm.bit_length()
    if l1 > l2:
        return s
    if l1 < l2:
        return 0
    # same binade: compare exactly at the common exponent
    k = min(e, re_)
    A = m << (e - k)
    B = rm << (re_ - k)
    return s if A > B else 0


def ucoset_ok(c, N):
    if N == 0:
        return c in (1, -1)
    return N >= 1 and 1 <= c <= N and gcd(c, N) == 1


def fmpq_ok(n, d):
    return d >= 1 and gcd(n, d) == 1


def parse_lb(t):
    p = t.h()
    f = t.kw("x", "b")
    return (p, f, t.h(), t.h(), t.h())


def lb_limits(lb, lim):
    p, f, a, b, c = lb
    vals = [c] if f == "x" else [b, c]
    for v in vals:
        if abs(v) > lim[2]:
            raise Fail(LIMIT)
        if not (SLMIN <= v <= SLMAX):
            raise Fail(LIMIT)       # decision 1 of the u-dump1 report (not in the conventions)


def lb_ok(lb):
    p, f, a, b, c = lb
    if not is_prime(p):
        return False
    if f == "x":
        num, den, v = a, b, c
        if not fmpq_ok(num, den):
            return False
        if num == 0:
            return v == 0
        return num % p != 0 and den % p != 0
    u, v, N = a, b, c
    if u == 0:
        return v == 0
    if not (v < N and u % p != 0 and u > 0):
        return False
    k = N - v
    if k >= u.bit_length():
        return True
    return u < p ** k


def ref(typ, s, lim=DEF):
    """typ in ucoset idele idclass lball sball; s bytes. returns status."""
    try:
        return _ref(typ, s, lim)
    except Fail as f:
        return f.st


def _ref(typ, s, lim):
    if len(s) > lim[0]:
        raise Fail(LIMIT)
    for b in s:
        if not (0x20 <= b <= 0x7E or b in (9, 10, 13)):
            raise Fail(PARSE)
    s = s.decode("ascii")
    m = re.match(r"adf([0-9]+)( |\Z)", s)
    if not m:
        raise Fail(PARSE)
    ver = m.group(1)
    if ver != "0" and ver.startswith("0"):
        raise Fail(PARSE)      # ambiguous: 10.1 gives no grammar for the version digits
    if ver != "1":
        raise Fail(UNSUPPORTED)
    rest = s[m.end():]
    f = re.match(r"([A-Z][^ ]*)( |\Z)", rest)
    if not f:
        raise Fail(PARSE)
    if f.group(1) != "Q":
        raise Fail(UNSUPPORTED)
    body = rest[f.end():]
    if f.group(2) == "":
        raise Fail(PARSE)
    toks = body.split(" ")
    if any(x == "" for x in toks):
        raise Fail(PARSE)
    t = T(toks)
    name = t.raw()
    if name != typ:
        raise Fail(PARSE)
    # ---- grammar (stage 3)
    if typ == "ucoset":
        v = (t.h(), t.h())
    elif typ == "idele":
        n = t.count_arch = t.h()
        if n < 0 or n * 4 > t.left():
            raise Fail(PARSE)
        arbs = [t.arb() for _ in range(n)]
        v = (n, arbs, t.h(), t.h(), t.h(), t.h())
    elif typ == "idclass":
        v = (t.arb(), t.h(), t.h())
    elif typ == "lball":
        v = (parse_lb(t),)
    elif typ == "sball":
        tag = t.kw("n", "r", "c")
        if tag == "n":
            ar = None
        elif tag == "r":
            ar = [t.arb()]
        else:
            ar = [t.arb(), t.arb()]
        cnt = t.h()
        if cnt < 0 or cnt * 5 > t.left():
            raise Fail(PARSE)
        lbs = [parse_lb(t) for _ in range(cnt)]
        v = (tag, ar, cnt, lbs)
    if t.left() != 0:
        raise Fail(PARSE)
    # ---- stage 4
    if typ == "sball" and v[2] > lim[3]:
        raise Fail(LIMIT)
    lbs = [v[0]] if typ == "lball" else (v[3] if typ == "sball" else [])
    for lb in lbs:
        lb_limits(lb, lim)
    # ---- stage 5
    for lb in lbs:
        if lb[0] >= WORD:
            raise Fail(UNSUPPORTED)
    # ---- stage 6
    if typ == "ucoset":
        if not ucoset_ok(*v):
            raise Fail(DOMAIN)
    elif typ == "idele":
        n, arbs, num, den, c, N = v
        if n != 1:
            raise Fail(DOMAIN)
        if not arb_ok(arbs[0]) or arb_sign(arbs[0]) == 0:
            raise Fail(DOMAIN)
        if not fmpq_ok(num, den) or num <= 0:
            raise Fail(DOMAIN)
        if not ucoset_ok(c, N):
            raise Fail(DOMAIN)
    elif typ == "idclass":
        a, c, N = v
        if not arb_ok(a) or arb_sign(a) != 1:
            raise Fail(DOMAIN)
        if not ucoset_ok(c, N):
            raise Fail(DOMAIN)
    elif typ == "lball":
        if not lb_ok(v[0]):
            raise Fail(DOMAIN)
    else:
        tag, ar, cnt, lbs = v
        if ar and not all(arb_ok(a) for a in ar):
            raise Fail(DOMAIN)
        prev = 0
        for lb in lbs:
            if not lb_ok(lb):
                raise Fail(DOMAIN)
            if lb[0] <= prev:
                raise Fail(DOMAIN)
            prev = lb[0]
    return OK
