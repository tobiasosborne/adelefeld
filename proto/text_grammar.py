#!/usr/bin/env python3
"""Reference parser and printer for the text forms of docs/conventions.md, sections 8 to 10.

This is the oracle for the C parser and printer (work package 1.4). It works with exact rationals only
(fractions.Fraction): a real ball in the value form is read into the exact interval [m - r, m + r]
(docs/conventions.md 9.5), never into a float.

Entry points:
  canonical(type_name, data)  -> canonical text, or "!STATUS"            (value form, sections 9.1 to 9.4)
  classify(data)              -> type name, or "!STATUS"                 (section 9.7)
  read_real(data)             -> (lo, hi) exact, raises TextError         (section 9.5)
  print_real(mid, rad, digits=20, cond=None) -> text                     (section 9.5)
  dump_roundtrip(data)        -> the re-dumped text, or "!STATUS"         (section 10)
  dump_contexts(data)         -> [(K, (q_1, ..., q_k)), ...], or "!STATUS"  (section 10.2, gate G3)
  dump_load_check(data, binds) -> "OK", or "!STATUS"                       (section 10.2, gate G3)
  modctx_new_from_dump(data, occurrence) -> (K, (q_1, ..., q_k)), or "!STATUS" (section 10.2, closure C3)
  combine(pairs)              -> (status, place)                          (section 3.3)

Limitations of the reference (not of the specification): the primitive character of a Dirichlet character
is found by search with python-flint and is refused (UNSUPPORTED) for moduli above 10^5; arb exponents above
2^20 in absolute value are refused (LIMIT) where the value of the ball must be computed. Value-text round trips
are exact here; a C parser stores rounded balls and its round trips may widen (gate G4, conventions 9.6).
"""
import math
import re
import sys
from fractions import Fraction
from math import gcd

if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

# docs/conventions.md 3.1 (CV-03)
STATUS = {"OK": 0, "NOT_DETERMINED": 1, "UNIT_NOT_CERTIFIED": 2, "NEEDS_SPLIT": 3, "NOT_UNIQUE": 4,
          "NO_SOLUTION": 5, "NOT_UNIT": 6, "DOMAIN": 7, "UNSUPPORTED": 8, "PARSE": 9, "LIMIT": 10}

TYPES = ["rat", "fball", "adele", "cadele", "ucoset", "idele", "idclass", "lball", "sball", "qclass", "ffun",
         "rfun", "char"]

WORD = 2 ** 64
MAG_LIMIT = 2 ** 30          # radius mantissa of an arb: MAG_BITS = 30 (/usr/include/flint/mag.h:117)


class TextError(Exception):
    def __init__(self, status):
        Exception.__init__(self, status)
        self.status = status


class Limits:
    """docs/conventions.md 8.4 (CV-27)."""

    def __init__(self, max_len=1048576, max_exp10=100000, max_prec=100000, max_items=1048576):
        self.max_len = max_len
        self.max_exp10 = max_exp10
        self.max_prec = max_prec
        self.max_items = max_items


DEFAULT_LIMITS = Limits()


def combine(pairs):
    """docs/conventions.md 3.3: pairs of (place, status name); place 0 is the archimedean place.
    Returns (combined status, first place in canonical order with that status), or ("OK", None)."""
    worst = "OK"
    for _, st in pairs:
        if STATUS[st] > STATUS[worst]:
            worst = st
    if worst == "OK":
        return "OK", None
    places = sorted(p for p, st in pairs if st == worst)
    return worst, places[0]


# ================================================================================================================
# exact decimal arithmetic for real balls (docs/conventions.md 9.5)

def _floor_log10(y):
    """X(y) = floor(log10 |y|) for a non-zero rational y, exactly."""
    y = abs(Fraction(y))
    est = int((y.numerator.bit_length() - y.denominator.bit_length()) * 0.30102999566398120)
    x = est - 2
    while Fraction(10) ** (x + 1) <= y:
        x += 1
    while Fraction(10) ** x > y:
        x -= 1
    return x


def _pow10(q):
    return Fraction(10) ** q


def _valuation(n, p):
    """v_p(n) for n != 0, by repeated squaring of p (fast for large valuations)."""
    if n == 0:
        raise ValueError("v_p(0)")
    v = 0
    powers = [p]
    while n % powers[-1] == 0:
        n //= powers[-1]
        v += 1 << (len(powers) - 1)
        powers.append(powers[-1] * powers[-1])
    for i in range(len(powers) - 1, -1, -1):
        if n % powers[i] == 0:
            n //= powers[i]
            v += 1 << i
    return v


def _decimal_parts(x):
    """For an exact decimal x != 0: (sign, D, E) with |x| = D 10^E, D not divisible by 10; None if x is not a
    finite decimal."""
    x = Fraction(x)
    sign = -1 if x < 0 else 1
    num, den = abs(x.numerator), x.denominator
    a = (den & -den).bit_length() - 1
    den >>= a
    b = _valuation(den, 5)
    den //= 5 ** b
    if den != 1:
        return None
    m = max(a, b)
    D = num * (2 ** (m - a)) * (5 ** (m - b))
    E = -m
    # strip trailing zeros; fast path by powers of 10
    while D % 10 == 0:
        k = 1
        while D % (10 ** (2 * k)) == 0:
            k *= 2
        D //= 10 ** k
        E += k
    return sign, D, E


def _sig_digits(x):
    if x == 0:
        return 0
    parts = _decimal_parts(x)
    if parts is None:
        return None
    return len(str(parts[1]))


def fmt_decimal(x):
    """fmt of docs/conventions.md 9.5 for an exact decimal."""
    x = Fraction(x)
    if x == 0:
        return "0"
    parts = _decimal_parts(x)
    if parts is None:
        raise ValueError("not a finite decimal: %r" % x)
    sign, D, E = parts
    s = str(D)
    k = len(s)
    X = E + k - 1
    if -4 <= X <= 20:
        if E >= 0:
            body = s + "0" * E
        elif k > -E:
            body = s[:k + E] + "." + s[k + E:]
        else:
            body = "0." + "0" * (-E - k) + s
    else:
        body = s[0] + ("." + s[1:] if k > 1 else "") + "e" + str(X)
    return ("-" if sign < 0 else "") + body


def _round_q(y, q):
    """y rounded to the nearest multiple of 10^q, ties to the even multiple (Fraction.__round__ is half-even)."""
    u = _pow10(q)
    return round(Fraction(y) / u) * u


def _ceil_k(E, k):
    if E == 0:
        return Fraction(0)
    unit = _pow10(_floor_log10(E) - k + 1)
    return math.ceil(Fraction(E) / unit) * unit


def _is_multiple(M, q):
    return (Fraction(M) / _pow10(q)).denominator == 1


def print_real_detail(mid, rad, n, k=2):
    """One level k of the algorithm of docs/conventions.md 9.5. Returns (text, loops, (M, R))."""
    mid, rad = Fraction(mid), Fraction(rad)
    if rad < 0:
        raise ValueError("negative radius")
    nk = n + k - 2
    if rad == 0:
        sd = _sig_digits(mid)
        if sd is not None and sd <= nk:
            return fmt_decimal(mid), 0, (mid, Fraction(0))
    if mid == 0:
        R = _ceil_k(rad, k)
        return "0 +/- " + fmt_decimal(R), 0, (Fraction(0), R)
    q = _floor_log10(mid) - nk + 1
    if rad > 0:
        q = max(q, _floor_log10(rad) - k + 1)
    loops = 0
    while True:
        loops += 1
        M = _round_q(mid, q)
        R = _ceil_k(rad + abs(M - mid), k)
        if R == 0:
            return fmt_decimal(M), loops, (M, R)
        if M == 0:
            q2 = _floor_log10(R) - k + 1
        else:
            q2 = max(_floor_log10(M) - nk + 1, _floor_log10(R) - k + 1)
        if _is_multiple(M, q2):
            return fmt_decimal(M) + " +/- " + fmt_decimal(R), loops, (M, R)
        q = q2


def _satisfies(lo, hi, cond):
    if cond is None:
        return True
    if cond == "nonzero":
        return lo > 0 or hi < 0
    if cond == "positive":
        return lo > 0
    raise ValueError(cond)


def print_real(mid, rad, digits=20, cond=None):
    """docs/conventions.md 9.5, with constrained printing for cond in (None, "nonzero", "positive")."""
    mid, rad = Fraction(mid), Fraction(rad)
    if not _satisfies(mid - rad, mid + rad, cond):
        raise ValueError("the ball itself violates the condition")
    if cond is None:
        return print_real_detail(mid, rad, digits, 2)[0]
    # constrained: least level k, then repeat on the value read back until the text is stable; the least level
    # decreases strictly until then, so this ends (docs/conventions.md 9.5)
    text = None
    while True:
        k = 2
        while True:
            t, _, (M, R) = print_real_detail(mid, rad, digits, k)
            if _satisfies(M - R, M + R, cond):
                break
            k += 1
        if t == text:
            return t
        text, mid, rad = t, M, R


def fmt_rat(x):
    x = Fraction(x)
    if x.denominator == 1:
        return str(x.numerator)
    return "%d/%d" % (x.numerator, x.denominator)


# ================================================================================================================
# value form: syntax (docs/conventions.md 9.1, 9.2)

RE_UINT = re.compile(r"[0-9]+")
RE_INT = re.compile(r"-?[0-9]+")
RE_URAT = re.compile(r"[0-9]+(?:/[0-9]+)?")
RE_RAT = re.compile(r"-?[0-9]+(?:/[0-9]+)?")
RE_UDEC = re.compile(r"[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?")
RE_DEC = re.compile(r"-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?")
RE_SINT = re.compile(r"-?[0-9]+")
RE_LETTERS = re.compile(r"[A-Za-z]+")
WS = " \t\n\r"


class Parser:
    def __init__(self, s):
        self.s = s
        self.i = 0

    def fail(self):
        raise TextError("PARSE")

    def ws(self):
        s, i = self.s, self.i
        while i < len(s) and s[i] in WS:
            i += 1
        self.i = i

    def at_end(self):
        self.ws()
        return self.i == len(self.s)

    def peek(self, tok):
        self.ws()
        if tok == "+" and self.s.startswith("+/-", self.i):
            return False
        return self.s.startswith(tok, self.i)

    def expect(self, tok):
        if not self.peek(tok):
            self.fail()
        self.i += len(tok)

    def peek_kw(self, word):
        self.ws()
        m = RE_LETTERS.match(self.s, self.i)
        return m is not None and m.group() == word

    def kw(self, word):
        if not self.peek_kw(word):
            self.fail()
        self.i += len(word)

    def num(self, rx):
        self.ws()
        m = rx.match(self.s, self.i)
        if m is None:
            self.fail()
        self.i = m.end()
        return m.group()

    # --- productions; each returns a raw node (tuples of strings) ---

    def real(self):
        mid = ("dec", self.num(RE_DEC))
        rad = None
        if self.peek("+/-"):
            self.expect("+/-")
            rad = ("dec", self.num(RE_UDEC))
        return ("real", mid, rad)

    def complex(self):
        self.expect("(")
        re_ = self.real()
        self.expect(")")
        self.expect("+")
        self.expect("(")
        im = self.real()
        self.expect(")")
        self.expect("*")
        self.kw("i")
        return ("complex", re_, im)

    def fin(self):
        a = self.num(RE_RAT)
        N = None
        if self.peek_kw("mod"):
            self.kw("mod")
            N = self.num(RE_URAT)
        return ("fin", a, N)

    def ucoset(self):
        self.expect("[")
        c = self.num(RE_INT)
        N = None
        if self.peek_kw("mod"):
            self.kw("mod")
            N = self.num(RE_UINT)
        self.expect("]")
        return ("ucoset", c, N)

    def lcoord(self, p):
        a = self.num(RE_RAT)
        if not self.peek("+"):
            return ("lcoord", p, a, None, None)
        self.expect("+")
        self.kw("O")
        self.expect("(")
        base = self.num(RE_UINT)
        exp = None
        if self.peek("^"):
            self.expect("^")
            exp = self.num(RE_SINT)
        self.expect(")")
        return ("lcoord", p, a, base, exp)

    def adele_v(self):
        self.expect("(")
        r = self.real()
        self.expect(";")
        f = self.fin()
        self.expect(")")
        return ("adele", r, f)

    def sentry(self):
        # place labels as tokens (seams R9): "inf" for the archimedean place, real or complex by the syntax
        if self.peek_kw("inf"):
            self.kw("inf")
            self.expect(":")
            if self.peek("("):
                return ("C", self.complex())
            return ("R", self.real())
        self.kw("p")
        self.expect("=")
        p = ("prime", self.num(RE_UINT))
        self.expect(":")
        return ("P", self.lcoord(p))

    def rterm(self):
        self.kw("term")
        self.expect("(")
        self.kw("P")
        self.expect("=")
        self.expect("[")
        coeffs = []
        if not self.peek("]"):
            coeffs.append(self.complex())
            while self.peek(","):
                self.expect(",")
                coeffs.append(self.complex())
        self.expect("]")
        parts = []
        for name in ("A", "B", "C"):
            self.expect(",")
            self.kw(name)
            self.expect("=")
            parts.append(self.complex())
        self.expect(")")
        return ("term", ("items", coeffs), parts[0], parts[1], parts[2])


def _syntax(type_name, s):
    """Stage 3: the raw tree of the start symbol type_name, or TextError("PARSE")."""
    P = Parser(s)
    if type_name == "rat":
        node = ("rat", P.num(RE_RAT))
    elif type_name == "fball":
        if P.peek("("):
            P.expect("(")
            P.expect("*")
            P.expect(";")
            f = P.fin()
            P.expect(")")
        else:
            a = P.num(RE_RAT)
            P.kw("mod")
            f = ("fin", a, P.num(RE_URAT))
        node = ("fball", f)
    elif type_name == "adele":
        node = P.adele_v()
    elif type_name == "cadele":
        P.expect("(")
        z = P.complex()
        P.expect(";")
        f = P.fin()
        P.expect(")")
        node = ("cadele", z, f)
    elif type_name == "ucoset":
        node = ("ucoset_v", P.ucoset())
    elif type_name == "idele":
        P.expect("(")
        r = P.real()
        P.expect(";")
        scale = P.num(RE_URAT)
        P.expect("*")
        u = P.ucoset()
        P.expect(")")
        node = ("idele", r, scale, u)
    elif type_name == "idclass":
        P.expect("<")
        r = P.real()
        P.expect(";")
        u = P.ucoset()
        P.expect(">")
        node = ("idclass", r, u)
    elif type_name == "lball":
        P.expect("[")
        P.kw("p")
        P.expect("=")
        p = ("prime", P.num(RE_UINT))
        P.expect(":")
        lc = P.lcoord(p)
        P.expect("]")
        node = ("lball", lc)
    elif type_name == "sball":
        P.expect("{")
        entries = []
        if not P.peek("}"):
            entries.append(P.sentry())
            while P.peek(";"):
                P.expect(";")
                entries.append(P.sentry())
        P.expect("}")
        node = ("sball", ("items", entries))
    elif type_name == "qclass":
        if P.peek_kw("union"):
            P.kw("union")
            P.expect("(")
            pieces = [P.adele_v()]
            while P.peek(","):
                P.expect(",")
                pieces.append(P.adele_v())
            P.expect(")")
            node = ("qunion", ("items", pieces))
        else:
            node = ("qlift", P.adele_v())
        P.expect("+")
        P.kw("Q")
    elif type_name == "ffun":
        P.kw("ffun")
        P.expect("(")
        P.kw("D")
        P.expect("=")
        D = P.num(RE_UINT)
        P.expect(",")
        P.kw("M")
        P.expect("=")
        M = P.num(RE_UINT)
        P.expect(";")
        vals = [P.complex()]
        while P.peek(","):
            P.expect(",")
            vals.append(P.complex())
        P.expect(")")
        node = ("ffun", D, M, ("items", vals))
    elif type_name == "rfun":
        P.kw("rfun")
        P.expect("(")
        terms = []
        if not P.peek(")"):
            terms.append(P.rterm())
            while P.peek(","):
                P.expect(",")
                terms.append(P.rterm())
        P.expect(")")
        node = ("rfun", ("items", terms))
    elif type_name == "char":
        P.kw("char")
        P.expect("(")
        P.kw("q")
        P.expect("=")
        q = P.num(RE_UINT)
        P.expect(",")
        P.kw("n")
        P.expect("=")
        n = P.num(RE_UINT)
        P.expect(",")
        P.kw("s")
        P.expect("=")
        z = P.complex()
        P.expect(")")
        node = ("char", ("charq", q), n, z)
    else:
        raise ValueError(type_name)
    if not P.at_end():
        P.fail()
    return node


def _prep(data, limits):
    """Stages 1 and 2 (docs/conventions.md 8.5)."""
    if isinstance(data, str):
        data = data.encode("utf-8")
    if len(data) > limits.max_len:
        raise TextError("LIMIT")
    for c in data:
        if not (0x20 <= c <= 0x7E or c in (0x09, 0x0A, 0x0D)):
            raise TextError("PARSE")
    return data.decode("ascii")


def _walk(node):
    yield node
    if isinstance(node, tuple):
        for child in node[1:]:
            if isinstance(child, (tuple, list)):
                for x in _walk(child):
                    yield x
    elif isinstance(node, list):
        for child in node:
            for x in _walk(child):
                yield x


def _strip_int(digits):
    d = digits.lstrip("0")
    return d if d else "0"


def _check_limits(tree, limits):
    """Stage 4, on digit strings, before big numbers are formed."""
    for node in _walk(tree):
        if not isinstance(node, tuple):
            continue
        tag = node[0]
        if tag == "dec":
            m = re.search(r"[eE]([+-]?)([0-9]+)$", node[1])
            if m:
                # decision M1-D7 (finding R4): compare the exponent with max_exp10 as a number
                # of any length; there is no hidden 18-digit bound. The exponent is non-negative,
                # so a negative max_exp10 admits no literal with an exponent.
                e = _strip_int(m.group(2))
                if int(e) > limits.max_exp10:
                    raise TextError("LIMIT")
        elif tag == "lcoord" and node[3] is not None and node[4] is not None:
            e = _strip_int(node[4].lstrip("-"))
            if len(e) > 18 or int(e) > limits.max_prec:
                raise TextError("LIMIT")
        elif tag == "items":
            if len(node[1]) > limits.max_items:
                raise TextError("LIMIT")
        elif tag == "ffun":
            D, M = _strip_int(node[1]), _strip_int(node[2])
            if len(D) + len(M) > 40 or int(D) * int(M) > limits.max_items:
                raise TextError("LIMIT")


def _check_unsupported(tree):
    """Stage 5: word restrictions."""
    for node in _walk(tree):
        if isinstance(node, tuple) and node[0] in ("prime", "charq"):
            v = _strip_int(node[1])
            if len(v) > 20 or int(v) >= WORD:
                raise TextError("UNSUPPORTED")


# ================================================================================================================
# value form: semantics (docs/conventions.md 9.3) and printing (9.4)

def _domain():
    raise TextError("DOMAIN")


def _rat(text):
    if "/" in text:
        a, b = text.split("/")
    else:
        a, b = text, "1"
    neg = a.startswith("-")
    a = int(_strip_int(a.lstrip("-")))
    b = int(_strip_int(b))
    if b == 0:
        _domain()
    return Fraction(-a if neg else a, b)


def _dec(text):
    m = re.fullmatch(r"(-?)([0-9]+)(?:\.([0-9]+))?(?:[eE]([+-]?[0-9]+))?", text)
    sign, ip, fp, ex = m.group(1), m.group(2), m.group(3) or "", m.group(4)
    digits = _strip_int(ip + fp)
    # A zero coefficient is the exact zero whatever the exponent (decision M1-D7, finding R4);
    # the power of ten is not formed, so a huge exponent within max_exp10 costs nothing.
    if digits == "0":
        return Fraction(0)
    e = int(ex) if ex else 0
    e -= len(fp)
    v = Fraction(int(digits)) * _pow10(e)
    return -v if sign else v


def _real(node):
    mid = _dec(node[1][1])
    rad = _dec(node[2][1]) if node[2] is not None else Fraction(0)
    return (mid, rad)


def _complex(node):
    return (_real(node[1]), _real(node[2]))


def _fin(node):
    a = _rat(node[1])
    N = _rat(node[2]) if node[2] is not None else Fraction(0)
    if N > 0:
        a = a - N * math.floor(a / N)
    return (a, N)


def _ucoset(node):
    c = int(node[1])
    N = int(_strip_int(node[2])) if node[2] is not None else 0
    if N == 0:
        if c not in (1, -1):
            _domain()
        return (c, 0)
    if gcd(c, N) != 1:
        _domain()
    c %= N
    if c == 0:
        c = N
    return (c, N)


def ucoset_normal(u):
    c, N = u
    if N == 0:
        return u
    if N % 4 == 2:
        N //= 2
    c %= N
    if c == 0:
        c = N
    return (c, N)


def _is_prime(p):
    if p < 2:
        return False
    import flint  # python-flint: FLINT's proven primality test (fmpz_is_prime, fmpz.h:760)
    return bool(flint.fmpz(p).is_prime())


def _lcoord(node):
    """Returns (p, exact, value): exact -> the rational; ball -> (centre c, N)."""
    p = int(_strip_int(node[1][1]))
    x = _rat(node[2])
    if not _is_prime(p):
        _domain()
    if node[3] is None:
        return (p, True, x)
    if int(_strip_int(node[3])) != p:
        _domain()
    N = int(node[4]) if node[4] is not None else 1
    return (p, False, (_padic_centre(x, p, N), N))


def _padic_centre(x, p, N):
    """The unique element of Z[1/p] in [0, p^N) in the ball x + p^N Z_p (docs/conventions.md 5.8)."""
    y = Fraction(x) / Fraction(p) ** N
    a, b = y.numerator, y.denominator
    j = 0
    while b % p == 0:
        b //= p
        j += 1
    if j == 0:
        return Fraction(0)
    pj = p ** j
    t = (a * pow(b, -1, pj)) % pj
    return Fraction(p) ** N * Fraction(t, pj)


def _fmt_fin(f):
    a, N = f
    return fmt_rat(a) if N == 0 else "%s mod %s" % (fmt_rat(a), fmt_rat(N))


def _fmt_ucoset(u):
    c, N = ucoset_normal(u)
    if N == 0:
        return "[%d]" % c
    return "[%d mod %d]" % (c, N)


def _fmt_real(r, cond=None):
    return print_real(r[0], r[1], 20, cond)


def _fmt_complex(z, cond_re=None):
    return "(%s) + (%s)*i" % (_fmt_real(z[0], cond_re), _fmt_real(z[1]))


def _fmt_lcoord(lc):
    p, exact, v = lc
    if exact:
        return fmt_rat(v)
    c, N = v
    return "%s + O(%d^%d)" % (fmt_rat(c), p, N)


def _is_exact_zero(z):
    """an acb that is the exact zero ball (both parts exactly 0; arb_is_zero)"""
    return z[0] == (Fraction(0), Fraction(0)) and z[1] == (Fraction(0), Fraction(0))


def _trim_zero_coeffs(coeffs):
    """docs/conventions.md 5.12 (gate finding G7): remove trailing exact zero coefficients; a ball merely
    containing zero is not trimmed."""
    while coeffs and _is_exact_zero(coeffs[-1]):
        coeffs.pop()
    return coeffs


def _check_real(r, cond):
    lo, hi = r[0] - r[1], r[0] + r[1]
    if not _satisfies(lo, hi, cond):
        _domain()


def _adele(node):
    return (_real(node[1]), _fin(node[2]))


def _fmt_adele(a):
    return "(%s ; %s)" % (_fmt_real(a[0]), _fmt_fin(a[1]))


def _primitive_char(q, n):
    """(conductor, Conrey label of the primitive character inducing (q, n)); search with python-flint."""
    import flint
    if q == 1:
        return (1, 1)
    if q > 10 ** 5:
        raise TextError("UNSUPPORTED")   # a limit of this reference only
    c = flint.dirichlet_char(q, n)
    f = int(c.conductor())
    if f == q:
        return (q, n)
    if f == 1:
        return (1, 1)
    xs = [x for x in range(1, q + 1) if gcd(x, q) == 1]
    vals = [complex(c(x)) for x in xs]
    for m in range(1, f + 1):
        if gcd(m, f) != 1:
            continue
        pm = flint.dirichlet_char(f, m)
        if all(abs(complex(pm(x)) - v) < 1e-9 for x, v in zip(xs, vals)):
            return (f, m)
    raise AssertionError("no primitive character found for (%d, %d)" % (q, n))


def _build_and_print(type_name, node):
    t = type_name
    if t == "rat":
        return fmt_rat(_rat(node[1]))
    if t == "fball":
        return "(* ; %s)" % _fmt_fin(_fin(node[1]))
    if t == "adele":
        return _fmt_adele(_adele(node))
    if t == "cadele":
        return "(%s ; %s)" % (_fmt_complex(_complex(node[1])), _fmt_fin(_fin(node[2])))
    if t == "ucoset":
        return _fmt_ucoset(_ucoset(node[1]))
    if t == "idele":
        r = _real(node[1])
        scale = _rat(node[2])
        u = _ucoset(node[3])
        _check_real(r, "nonzero")
        if scale <= 0:
            _domain()
        return "(%s ; %s * %s)" % (_fmt_real(r, "nonzero"), fmt_rat(scale), _fmt_ucoset(u))
    if t == "idclass":
        r = _real(node[1])
        u = _ucoset(node[2])
        _check_real(r, "positive")
        return "<%s ; %s>" % (_fmt_real(r, "positive"), _fmt_ucoset(u))
    if t == "lball":
        lc = _lcoord(node[1])
        return "[p=%d: %s]" % (lc[0], _fmt_lcoord(lc))
    if t == "sball":
        arch = None
        primes = {}
        for e in node[1][1]:
            if e[0] in ("R", "C"):
                if arch is not None:
                    _domain()
                arch = (e[0], _real(e[1]) if e[0] == "R" else _complex(e[1]))
            else:
                lc = _lcoord(e[1])
                if lc[0] in primes:
                    _domain()
                primes[lc[0]] = lc
        out = []
        if arch is not None:
            out.append("inf: " + (_fmt_real(arch[1]) if arch[0] == "R" else _fmt_complex(arch[1])))
        for p in sorted(primes):
            out.append("p=%d: %s" % (p, _fmt_lcoord(primes[p])))
        return "{" + "; ".join(out) + "}"
    if t == "qclass":
        if node[0] == "qlift":
            return _fmt_adele(_adele(node[1])) + " + Q"
        printed = {}
        for pn in node[1][1]:
            (mid, rad), (a, N) = _adele(pn)
            if not (0 <= mid <= 1):
                _domain()
            if a.denominator != 1 or N.denominator != 1:
                _domain()
            # order by the printed real part, so that the printed text is a fixed point (docs/conventions.md 9.4)
            text = _fmt_adele(((mid, rad), (a, N)))
            lo, hi = read_real(text[1:].split(" ; ")[0].encode())
            printed[(lo, hi, N, a)] = text
        return "union(" + ", ".join(printed[k] for k in sorted(printed)) + ") + Q"
    if t == "ffun":
        D, M = int(_strip_int(node[1])), int(_strip_int(node[2]))
        vals = [_complex(z) for z in node[3][1]]
        if D < 1 or M < 1 or len(vals) != D * M:
            _domain()
        return "ffun(D=%d, M=%d; %s)" % (D, M, ", ".join(_fmt_complex(z) for z in vals))
    if t == "rfun":
        terms = []
        for tn in node[1][1]:
            coeffs = _trim_zero_coeffs([_complex(z) for z in tn[1][1]])
            A, B, C = _complex(tn[2]), _complex(tn[3]), _complex(tn[4])
            _check_real(A[0], "positive")
            terms.append("term(P=[%s], A=%s, B=%s, C=%s)" % (
                ", ".join(_fmt_complex(z) for z in coeffs), _fmt_complex(A, "positive"), _fmt_complex(B),
                _fmt_complex(C)))
        return "rfun(" + ", ".join(terms) + ")"
    if t == "char":
        q = int(_strip_int(node[1][1]))
        n = int(_strip_int(node[2]))
        z = _complex(node[3])
        if q == 0:
            _domain()
        n %= q
        if gcd(n, q) != 1:
            _domain()
        if n == 0:
            n = q
        f, m = _primitive_char(q, n)
        return "char(q=%d, n=%d, s=%s)" % (f, m, _fmt_complex(z))
    raise ValueError(t)


def canonical(type_name, data, limits=DEFAULT_LIMITS):
    """The canonical text of `data` read as `type_name`, or "!STATUS" (docs/conventions.md 8.5)."""
    try:
        s = _prep(data, limits)
        tree = _syntax(type_name, s)
        _check_limits(tree, limits)
        _check_unsupported(tree)
        return _build_and_print(type_name, tree)
    except TextError as e:
        return "!" + e.status


def classify(data, limits=DEFAULT_LIMITS):
    """docs/conventions.md 9.7: stages 1 to 3 only."""
    try:
        s = _prep(data, limits)
    except TextError as e:
        return "!" + e.status
    found = []
    for t in TYPES:
        try:
            _syntax(t, s)
            found.append(t)
        except TextError:
            pass
    if len(found) > 1:
        raise AssertionError("languages not disjoint: %r in %r" % (s, found))
    return found[0] if found else "!PARSE"


def read_real(data, limits=DEFAULT_LIMITS):
    """The exact interval of a real ball in the value form; raises TextError."""
    s = _prep(data, limits)
    P = Parser(s)
    node = P.real()
    if not P.at_end():
        P.fail()
    _check_limits(node, limits)
    mid, rad = _real(node)
    return (mid - rad, mid + rad)


# ================================================================================================================
# dump form (docs/conventions.md 10)

RE_HEX = re.compile(r"(0|-?[1-9a-f][0-9a-f]*)\Z")


class Tokens:
    def __init__(self, toks):
        self.t = toks
        self.i = 0

    def left(self):
        return len(self.t) - self.i

    def word(self, *allowed):
        if self.i >= len(self.t) or self.t[self.i] not in allowed:
            raise TextError("PARSE")
        self.i += 1
        return self.t[self.i - 1]

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else None

    def h(self):
        if self.i >= len(self.t) or not RE_HEX.match(self.t[self.i]):
            raise TextError("PARSE")
        self.i += 1
        return int(self.t[self.i - 1], 16)

    def count(self, per_item):
        n = self.h()
        if n < 0 or n * per_item > self.left():
            raise TextError("PARSE")
        return n


def _hx(n):
    return ("-" if n < 0 else "") + format(abs(n), "x")


def _d_arb(T):
    return ("arb", T.h(), T.h(), T.h(), T.h())


def _d_arch(T, per):
    """A count of archimedean components (seams R3), then that many balls of `per` tokens; 1 for Q."""
    n = T.count(per)
    return [(_d_arb(T) if per == 4 else _d_acb(T)) for _ in range(n)]


def _d_acb(T):
    return ("acb", _d_arb(T), _d_arb(T))


def _d_ctx(T):
    K = T.h()
    k = T.count(1)
    return ("ctx", K, [T.h() for _ in range(k)])


def _d_fb(T):
    form = T.word("g", "l")
    if form == "g":
        return ("fb", "g", T.h(), T.h(), T.h())
    d = T.h()
    ctx = _d_ctx(T)
    if T.left() < len(ctx[2]):
        raise TextError("PARSE")
    return ("fb", "l", d, ctx, [T.h() for _ in range(len(ctx[2]))])


def _d_lb(T):
    p = T.h()
    form = T.word("x", "b")
    return ("lb", p, form, T.h(), T.h(), T.h())


def _dump_syntax(s, limits):
    if len(s) > limits.max_len:
        raise TextError("LIMIT")
    if isinstance(s, str):
        s = s.encode("utf-8")
    for c in s:
        if not 0x20 <= c <= 0x7E:
            raise TextError("PARSE")
    s = s.decode("ascii")
    m = re.match(r"adf([0-9]+)( |\Z)", s)
    if m is None:
        raise TextError("PARSE")
    ver = m.group(1)
    if ver != "0" and ver.startswith("0"):
        raise TextError("PARSE")
    if ver != "1":
        raise TextError("UNSUPPORTED")
    # the field descriptor (seams R9): an upper-case letter and further non-space characters; "Q" in version 1
    f = re.match(r"([A-Z][^ ]*)( |\Z)", s[m.end():])
    if f is None:
        raise TextError("PARSE")
    if f.group(1) != "Q":
        raise TextError("UNSUPPORTED")
    toks = s[m.end() + f.end():].split(" ")
    if any(t == "" for t in toks):
        raise TextError("PARSE")
    T = Tokens(toks)
    kind = T.word("rat", "fball", "scaled", "adele", "cadele", "ucoset", "idele", "idclass", "lball", "sball",
                  "qclass", "ffun", "rfun", "char", "modctx")
    if kind == "rat":
        node = (kind, T.h(), T.h())
    elif kind == "fball":
        node = (kind, _d_fb(T))
    elif kind == "scaled":
        form = T.word("x", "s")
        if form == "x":
            node = (kind, "x", T.h(), T.h(), None, _d_ctx(T))
        else:
            node = (kind, "s", T.h(), T.h(), T.h(), _d_ctx(T))
    elif kind == "adele":
        node = (kind, _d_arch(T, 4), _d_fb(T))
    elif kind == "cadele":
        node = (kind, _d_arch(T, 8), _d_fb(T))
    elif kind == "ucoset":
        node = (kind, T.h(), T.h())
    elif kind == "idele":
        node = (kind, _d_arch(T, 4), T.h(), T.h(), T.h(), T.h())
    elif kind == "idclass":
        node = (kind, _d_arb(T), T.h(), T.h())
    elif kind == "lball":
        node = (kind, _d_lb(T))
    elif kind == "sball":
        form = T.word("n", "r", "c")
        arch = None if form == "n" else (_d_arb(T) if form == "r" else _d_acb(T))
        n = T.count(5)
        node = (kind, form, arch, [_d_lb(T) for _ in range(n)])
    elif kind == "qclass":
        form = T.word("lift", "pieces")
        if form == "lift":
            node = (kind, form, [(_d_arch(T, 4), _d_fb(T))])
        else:
            n = T.count(8)
            node = (kind, form, [(_d_arch(T, 4), _d_fb(T)) for _ in range(n)])
    elif kind == "ffun":
        D, M = T.h(), T.h()
        if D < 0 or M < 0 or D * M * 8 > T.left():
            raise TextError("PARSE")
        node = (kind, D, M, [_d_acb(T) for _ in range(D * M)])
    elif kind == "rfun":
        n = T.count(25)
        terms = []
        for _ in range(n):
            L = T.count(8)
            coeffs = [_d_acb(T) for _ in range(L)]
            terms.append((coeffs, _d_acb(T), _d_acb(T), _d_acb(T)))
        node = (kind, terms)
    elif kind == "char":
        node = (kind, T.h(), T.h(), _d_acb(T))
    else:  # modctx
        node = (kind, _d_ctx(T))
    if T.left() != 0:
        raise TextError("PARSE")
    return node


def _arb_value(a, limits=None):
    """Exact (mid, rad) of a validated arb token group; refuses huge exponents (reference limit)."""
    _, mm, me, rm, re_ = a
    if abs(me) > 2 ** 20 or abs(re_) > 2 ** 20:
        raise TextError("LIMIT")
    return (Fraction(mm) * Fraction(2) ** me, Fraction(rm) * Fraction(2) ** re_)


def _v_arb(a):
    _, mm, me, rm, re_ = a
    if mm == 0:
        if me != 0:
            _domain()          # a special value (inf, nan, or unknown): arb_load_str would abort on some
    elif mm % 2 == 0:
        _domain()
    if rm == 0:
        if re_ != 0:
            _domain()
    elif rm < 0 or rm % 2 == 0 or rm >= MAG_LIMIT:
        _domain()


def _arb_sign(a):
    """+1 if the ball is positive, -1 if negative, 0 if it contains 0; exact, also for huge exponents."""
    _, mm, me, rm, re_ = a
    if mm == 0:
        return 0
    if rm == 0:
        return 1 if mm > 0 else -1
    # compare |mm| 2^me with rm 2^re
    d = me - re_
    am = abs(mm)
    if d >= 0:
        if d > rm.bit_length() + 1:
            bigger = True
        else:
            bigger = (am << d) > rm
    else:
        if -d > am.bit_length() + 1:
            bigger = False
        else:
            bigger = am > (rm << -d)
    if not bigger:
        return 0
    return 1 if mm > 0 else -1


def _one(arch):
    """Q has exactly one archimedean component."""
    if len(arch) != 1:
        _domain()
    return arch[0]


def _v_ctx(ctx, need_blocks):
    _, K, q = ctx
    if K < 1:
        _domain()
    if need_blocks and len(q) == 0:
        _domain()
    if q:
        prod = 1
        for i, a in enumerate(q):
            if not 2 <= a < WORD:
                _domain()
            for b in q[:i]:
                if gcd(a, b) != 1:
                    _domain()
            prod *= a
        if prod != K:
            _domain()


def _crt(res, q):
    A, M = 0, 1
    for r, m in zip(res, q):
        t = ((r - A) * pow(M, -1, m)) % m
        A += M * t
        M *= m
    return A % M


def _v_G(A, H, d):
    if d <= 0 or H < 0:
        _domain()
    if H > 0:
        if not 0 <= A < H or gcd(gcd(A, H), d) != 1:
            _domain()
    elif gcd(A, d) != 1:
        _domain()


def _v_fb(fb):
    """Validates and returns (A, H, d) of the represented set."""
    if fb[1] == "g":
        _, _, A, H, d = fb
        _v_G(A, H, d)
        return (A, H, d)
    _, _, d, ctx, res = fb
    _v_ctx(ctx, True)
    q = ctx[2]
    if d < 1:
        _domain()
    for r, m in zip(res, q):
        if not 0 <= r < m:
            _domain()
    # raw local value (docs/conventions.md 5.3; policies.md Lemma 17, Proposition 24): the data are unique in
    # the context without the gcd condition; the canonical triple is derived
    A, H = _crt(res, q), ctx[1]
    g = gcd(gcd(A, H), d)
    return (A // g, H // g, d // g)


def _v_fmpq(num, den):
    if den < 1 or gcd(abs(num), den) != 1:
        _domain()


def _v_ucoset(c, N):
    if N == 0:
        if c not in (1, -1):
            _domain()
    elif N < 0 or not 1 <= c <= N or gcd(c, N) != 1:
        _domain()


def _v_lb(lb, limits):
    _, p, form, a, b, c = lb
    if form == "x":
        num, den, v = a, b, c
        if abs(v) > limits.max_prec:
            raise TextError("LIMIT")
    else:
        u, v, N = a, b, c
        if abs(v) > limits.max_prec or abs(N) > limits.max_prec:
            raise TextError("LIMIT")
    if p >= WORD:
        raise TextError("UNSUPPORTED")
    if not _is_prime(p):
        _domain()
    if form == "x":
        _v_fmpq(num, den)
        if num == 0:
            if v != 0:
                _domain()
        elif num % p == 0 or den % p == 0:
            _domain()
    else:
        if u == 0:
            if v != 0:
                _domain()
        elif not (v < N and u % p != 0 and 0 < u < p ** (N - v)):
            _domain()


def _ctx_occurrences(node):
    """Every "ctx" node of a parsed dump, in dump traversal order (docs/conventions.md 10.2, gate finding G3)."""
    if isinstance(node, tuple):
        if node[0] == "ctx":
            yield node
            return
        for child in node[1:]:
            for x in _ctx_occurrences(child):
                yield x
    elif isinstance(node, list):
        for child in node:
            for x in _ctx_occurrences(child):
                yield x


def _d_is_exact_zero(z):
    """an acb token group that is the exact zero ball"""
    return all(a[1] == 0 and a[3] == 0 for a in z[1:])


def _dump_validate(node, limits):
    kind = node[0]
    # stage 4 and 5 first, then stage 6
    if kind == "sball" and len(node[3]) > limits.max_items:
        raise TextError("LIMIT")
    if kind in ("qclass",) and len(node[2]) > limits.max_items:
        raise TextError("LIMIT")
    if kind == "ffun" and node[1] * node[2] > limits.max_items:
        raise TextError("LIMIT")
    if kind == "rfun" and (len(node[1]) > limits.max_items or any(len(t[0]) > limits.max_items
                                                                   for t in node[1])):
        raise TextError("LIMIT")
    # stage 4 (gate finding G8): the block count of every context occurrence, including contexts nested in
    # finite balls, adeles and quotient pieces; checked before any semantic check, whatever fails later
    for ctx in _ctx_occurrences(node):
        if len(ctx[2]) > limits.max_items:
            raise TextError("LIMIT")
    lbs = [node[1]] if kind == "lball" else (node[3] if kind == "sball" else [])
    for lb in lbs:                      # stage 4 for every local ball before stage 5 for any
        vals = [lb[5]] if lb[2] == "x" else [lb[4], lb[5]]
        if any(abs(v) > limits.max_prec for v in vals):
            raise TextError("LIMIT")
    for lb in lbs:
        if lb[1] >= WORD:
            raise TextError("UNSUPPORTED")
    if kind == "char" and node[1] >= WORD:
        raise TextError("UNSUPPORTED")
    # stage 6
    if kind == "rat":
        _v_fmpq(node[1], node[2])
    elif kind == "fball":
        _v_fb(node[1])
    elif kind == "scaled":
        _, form, num, den, u, ctx = node
        _v_ctx(ctx, False)
        _v_fmpq(num, den)
        if form == "s" and (num <= 0 or not 0 <= u < ctx[1]):
            _domain()
    elif kind == "adele":
        _v_arb(_one(node[1]))
        _v_fb(node[2])
    elif kind == "cadele":
        z = _one(node[1])
        _v_arb(z[1])
        _v_arb(z[2])
        _v_fb(node[2])
    elif kind == "ucoset":
        _v_ucoset(node[1], node[2])
    elif kind == "idele":
        _v_arb(_one(node[1]))
        if _arb_sign(_one(node[1])) == 0:
            _domain()
        _v_fmpq(node[2], node[3])
        if node[2] <= 0:
            _domain()
        _v_ucoset(node[4], node[5])
    elif kind == "idclass":
        _v_arb(node[1])
        if _arb_sign(node[1]) != 1:
            _domain()
        _v_ucoset(node[2], node[3])
    elif kind == "lball":
        _v_lb(node[1], limits)
    elif kind == "sball":
        _, form, arch, lbs = node
        if form == "r":
            _v_arb(arch)
        elif form == "c":
            _v_arb(arch[1])
            _v_arb(arch[2])
        prev = 0
        for lb in lbs:
            _v_lb(lb, limits)
            if lb[1] <= prev:
                _domain()
            prev = lb[1]
    elif kind == "qclass":
        _, form, pieces = node
        if form == "lift":
            _v_arb(_one(pieces[0][0]))
            _v_fb(pieces[0][1])
        else:
            if len(pieces) == 0:
                _domain()
            keys = []
            for arbs, fb in pieces:
                arb = _one(arbs)
                _v_arb(arb)
                A, H, d = _v_fb(fb)
                mid, rad = _arb_value(arb)
                if not 0 <= mid <= 1 or d != 1:
                    _domain()
                keys.append((mid - rad, mid + rad, H, A))
            if any(keys[i] >= keys[i + 1] for i in range(len(keys) - 1)):
                _domain()
    elif kind == "ffun":
        _, D, M, vals = node
        if D < 1 or M < 1:
            _domain()
        for z in vals:
            _v_arb(z[1])
            _v_arb(z[2])
    elif kind == "rfun":
        for coeffs, A, B, C in node[1]:
            for z in coeffs + [A, B, C]:
                _v_arb(z[1])
                _v_arb(z[2])
            if coeffs and _d_is_exact_zero(coeffs[-1]):
                _domain()          # nonempty P with an exact-zero last coefficient is not normalized (G7)
            if _arb_sign(A[1]) != 1:
                _domain()
    elif kind == "char":
        _, q, n, z = node
        if q < 1 or not 1 <= n <= q or gcd(n, q) != 1:
            _domain()
        if _primitive_char(q, n) != (q, n):
            _domain()
        _v_arb(z[1])
        _v_arb(z[2])
    elif kind == "modctx":
        _v_ctx(node[1], False)


def _p_arb(a):
    return " ".join(_hx(x) for x in a[1:])


def _p_acb(z):
    return _p_arb(z[1]) + " " + _p_arb(z[2])


def _p_arch(arch):
    return " ".join([_hx(len(arch))] + [(_p_arb(a) if a[0] == "arb" else _p_acb(a)) for a in arch])


def _p_ctx(ctx):
    return " ".join([_hx(ctx[1]), _hx(len(ctx[2]))] + [_hx(x) for x in ctx[2]])


def _p_fb(fb):
    if fb[1] == "g":
        return "g " + " ".join(_hx(x) for x in fb[2:])
    _, _, d, ctx, res = fb
    return " ".join(["l", _hx(d), _p_ctx(ctx)] + [_hx(r) for r in res])


def _p_lb(lb):
    _, p, form, a, b, c = lb
    return " ".join([_hx(p), form, _hx(a), _hx(b), _hx(c)])


def _dump_print(node):
    kind = node[0]
    out = ["adf1", "Q", kind]
    if kind in ("rat", "ucoset"):
        out += [_hx(node[1]), _hx(node[2])]
    elif kind == "fball":
        out.append(_p_fb(node[1]))
    elif kind == "scaled":
        _, form, num, den, u, ctx = node
        out += [form, _hx(num), _hx(den)] + ([] if form == "x" else [_hx(u)]) + [_p_ctx(ctx)]
    elif kind in ("adele", "cadele"):
        out += [_p_arch(node[1]), _p_fb(node[2])]
    elif kind == "idele":
        out += [_p_arch(node[1])] + [_hx(x) for x in node[2:]]
    elif kind == "idclass":
        out += [_p_arb(node[1]), _hx(node[2]), _hx(node[3])]
    elif kind == "lball":
        out.append(_p_lb(node[1]))
    elif kind == "sball":
        _, form, arch, lbs = node
        out.append(form)
        if form == "r":
            out.append(_p_arb(arch))
        elif form == "c":
            out.append(_p_acb(arch))
        out.append(_hx(len(lbs)))
        out += [_p_lb(lb) for lb in lbs]
    elif kind == "qclass":
        _, form, pieces = node
        out.append(form)
        if form == "pieces":
            out.append(_hx(len(pieces)))
        out += [_p_arch(a) + " " + _p_fb(f) for a, f in pieces]
    elif kind == "ffun":
        out += [_hx(node[1]), _hx(node[2])] + [_p_acb(z) for z in node[3]]
    elif kind == "rfun":
        out.append(_hx(len(node[1])))
        for coeffs, A, B, C in node[1]:
            out.append(_hx(len(coeffs)))
            out += [_p_acb(z) for z in coeffs] + [_p_acb(A), _p_acb(B), _p_acb(C)]
    elif kind == "char":
        out += [_hx(node[1]), _hx(node[2]), _p_acb(node[3])]
    elif kind == "modctx":
        out.append(_p_ctx(node[1]))
    return " ".join(out)


def dump_roundtrip(data, limits=DEFAULT_LIMITS):
    """Load a dump strictly and dump it again; or "!STATUS"."""
    try:
        node = _dump_syntax(data, limits)
        _dump_validate(node, limits)
        return _dump_print(node)
    except TextError as e:
        return "!" + e.status


def dump_contexts(data, limits=DEFAULT_LIMITS):
    """docs/conventions.md 10.2 (gate finding G3): the context occurrences of a dump, in dump traversal order,
    each as (K, (q_1, ..., q_k)); no value is constructed. "!STATUS" for an invalid dump."""
    try:
        node = _dump_syntax(data, limits)
        _dump_validate(node, limits)
        return [(ctx[1], tuple(ctx[2])) for ctx in _ctx_occurrences(node)]
    except TextError as e:
        return "!" + e.status


def dump_load_check(data, binds, limits=DEFAULT_LIMITS):
    """docs/conventions.md 10.2 (gate finding G3): the binding rule of the loader. `binds` stands for the array
    of caller-owned context bindings, one per context occurrence in dump traversal order; an entry is None (a
    missing binding) or (K, (q_1, ..., q_k)) matching the modulus and ordered blocks of the caller's context.
    Returns "OK" if `data` loads with these bindings, else "!STATUS". The pointer identity that identical()
    additionally requires (10.2) is not modelled here."""
    occ = dump_contexts(data, limits)
    if isinstance(occ, str):
        return occ
    if len(binds) != len(occ):
        return "!DOMAIN"
    for b, desc in zip(binds, occ):
        if b is None or len(b) != 2 or b[0] != desc[0] or tuple(b[1]) != desc[1]:
            return "!DOMAIN"
    return "OK"


def modctx_new_from_dump(data, occurrence, limits=DEFAULT_LIMITS):
    """docs/conventions.md 10.2 (closure C3): `adf_modctx_new_from_dump(out, s, len, occurrence, lim)`
    constructs the context recorded at one occurrence of a dump and returns it as (K, (q_1, ..., q_k)), or
    "!STATUS". `occurrence` is 0-based in dump traversal order (a `modctx` dump has exactly one). The whole
    dump is validated in the order of 8.5 before the occurrence index is checked or a context is allocated;
    statuses are OK, PARSE, LIMIT, UNSUPPORTED, DOMAIN (conventions 3.2)."""
    occ = dump_contexts(data, limits)
    if isinstance(occ, str):
        return occ
    if not 0 <= occurrence < len(occ):
        return "!DOMAIN"
    return occ[occurrence]


# ================================================================================================================
# additive character on a finite ball (docs/conventions.md 6.1; docs/proofs/analysis.md Lemma 2)

def psi_phases(data, limits=DEFAULT_LIMITS):
    """The finite phases of psi on the finite ball `data` (value form of adf_fball): psi_f(a + N Zhat) is
    {E(a)} for N = 0 and {E(a) E(k/B) : 0 <= k < B} for N = A/B in lowest terms. Returns the angles t in [0, 1)
    of E(t), increasing, or "!STATUS"."""
    try:
        s = _prep(data, limits)
        tree = _syntax("fball", s)
        _check_limits(tree, limits)
        a, N = _fin(tree[1])
        B = N.denominator if N != 0 else 1
        if B > limits.max_items:
            raise TextError("LIMIT")
        angles = sorted({(a + Fraction(k, B)) % 1 for k in range(B)})
        return " ".join(fmt_rat(t) for t in angles)
    except TextError as e:
        return "!" + e.status
