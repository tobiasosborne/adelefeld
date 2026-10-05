#!/usr/bin/env python3
"""lanes/q-review2/text_oracle.py: my own reader and printer of the lift form `(r ; F) + Q` of
docs/conventions.md 9.2 to 9.6, compared with src/text.c (through text_eval) and with the
project reference proto/text_grammar.py.

Every expected status in TABLE is written from conventions 8.2 to 8.5 and 9.2 to 9.3, not from
the C code. Exact rationals throughout.

Usage: timeout 170 python3 text_oracle.py
"""
import os
import re
import subprocess
import sys
from fractions import Fraction as F

sys.path.insert(0, 'proto')
import text_grammar as tg                                   # noqa: E402

UINT = r'[0-9]+'
DEC = r'-?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?'
RE_REAL = re.compile(r'\s*(' + DEC + r')(?:\s*\+/-\s*(' + DEC + r'))?')
RE_RAT = re.compile(r'\s*(-?[0-9]+)(?:\s*/\s*([0-9]+))?')
RE_URAT = re.compile(r'\s*([0-9]+)(?:\s*/\s*([0-9]+))?')


class Bad(Exception):
    def __init__(self, status):
        self.status = status
        super().__init__(status)


class Reader:
    """Stage 3 of conventions 8.5 for the start symbol qclass_v."""

    def __init__(self, s):
        self.s = s
        self.i = 0

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] in ' \t\n\r':
            self.i += 1

    def lit(self, t):
        self.ws()
        if not self.s.startswith(t, self.i):
            raise Bad('PARSE')
        self.i += len(t)

    def kw(self, t):
        self.ws()
        j = self.i
        while j < len(self.s) and self.s[j].isalpha() and self.s[j].isascii():
            j += 1
        if self.s[self.i:j] != t:
            raise Bad('PARSE')
        self.i = j

    def real(self):
        m = RE_REAL.match(self.s, self.i)
        if not m:
            raise Bad('PARSE')
        self.i = m.end()
        return (dec_value(m.group(1)), dec_value(m.group(2)) if m.group(2) else F(0))

    def fin(self):
        m = RE_RAT.match(self.s, self.i)
        if not m:
            raise Bad('PARSE')
        self.i = m.end()
        den = F(int(m.group(2))) if m.group(2) else F(1)
        if den == 0:
            raise Bad('DOMAIN')
        a = F(int(m.group(1))) / den
        self.ws()
        if self.s.startswith('mod', self.i) and not self.s[self.i + 3:self.i + 4].isalpha():
            self.i += 3
            m2 = RE_URAT.match(self.s, self.i)
            if not m2:
                raise Bad('PARSE')
            self.i = m2.end()
            nden = F(int(m2.group(2))) if m2.group(2) else F(1)
            if nden == 0:
                raise Bad('DOMAIN')
            N = F(int(m2.group(1))) / nden
        else:
            N = F(0)
        return canonical_fin(a, N)

    def at_end(self):
        self.ws()
        return self.i == len(self.s)

    def qclass(self):
        self.ws()
        if self.s.startswith('union', self.i):
            self.i += 5
            self.lit('(')
            n = 0
            while True:
                self.adele()
                n += 1
                self.ws()
                if self.s.startswith(',', self.i):
                    self.i += 1
                    continue
                break
            self.lit(')')
            self.lit('+')
            self.kw('Q')
            if not self.at_end():
                raise Bad('PARSE')
            return ('union', n)
        self.adele()
        self.lit('+')
        self.kw('Q')
        if not self.at_end():
            raise Bad('PARSE')
        return ('lift', 1)

    def adele(self):
        self.lit('(')
        r = self.real()
        self.lit(';')
        f = self.fin()
        self.lit(')')
        return (r, f)


def bitlen2(y):
    """floor(log2 |y|) for a positive exact rational."""
    y = abs(y)
    return (y.numerator // y.denominator).bit_length()


def _fr(x):
    return x if isinstance(x, F) else F(x)


def dec_value(t):
    """The exact rational of a dec token (conventions 9.5, reading)."""
    t = t.strip()
    neg = t.startswith('-')
    if neg:
        t = t[1:]
    e = 0
    for k, ch in enumerate(t):
        if ch in 'eE':
            e = int(t[k + 1:])
            t = t[:k]
            break
    if '.' in t:
        ip, fp = t.split('.')
        dig = ip + fp
        e -= len(fp)
    else:
        dig = t
    v = F(int(dig)) * F(10) ** e if e >= 0 else F(int(dig), 10 ** (-e))
    return -v if neg else v


def dec_exponent_digits(t):
    for k, ch in enumerate(t):
        if ch in 'eE':
            return t[k + 1:]
    return None


def canonical_fin(a, N):
    """conventions 9.3: the centre reduced into [0, N), `mod 0` dropped, exact rational reduced."""
    if N == 0:
        return (a, N)
    c = a - N * (a // N)
    return (c, N)


def fin_text(f):
    a, N = f
    if N == 0:
        return fmt_rat(a)
    return "%s mod %s" % (fmt_rat(a), fmt_rat(N))


def fmt_rat(x):
    if x.denominator == 1:
        return str(x.numerator)
    return "%d/%d" % (x.numerator, x.denominator)


# ------------------------------------------------------------------ printing, conventions 9.5

def X(y):
    """floor(log10 |y|) for y != 0."""
    y = _fr(y)
    if y == 0:
        raise AssertionError("X(0) is not defined in conventions 9.5")
    y = abs(y)
    k = len(str(y.numerator)) - len(str(y.denominator))
    while F(10) ** k > y:
        k -= 1
    while F(10) ** (k + 1) <= y:
        k += 1
    return k


def ceil2(E):
    """ceil2 of conventions 9.5: E rounded up to two significant digits."""
    E = _fr(E)
    if E <= 0:
        return E
    k = X(E) - 1
    t = E * F(10) ** (-k)
    return F(-((-t.numerator) // t.denominator)) * F(10) ** k


def round_q(y, q):
    """y rounded to the nearest multiple of 10^q, ties to the even multiple."""
    if q >= 0:
        t = y / (F(10) ** q)
    else:
        t = y * (F(10) ** (-q))
    n, d = t.numerator, t.denominator
    s = -1 if n < 0 else 1
    n, d = abs(n), abs(d)
    quo, rem = divmod(n, d)
    if 2 * rem > d or (2 * rem == d and quo % 2 == 1):
        quo += 1
    r = F(s * quo)
    return r * (F(10) ** q if q >= 0 else F(1, 10 ** (-q)))


def sig_digits(x):
    """Number of significant decimal digits of the exact rational x, or None if it has none
    (a non-terminating decimal)."""
    x = _fr(x)
    num, den = abs(x.numerator), x.denominator
    if num == 0:
        return 1
    n = 0
    while den % 2 == 0:
        den //= 2
        n += 1
    m = 0
    while den % 5 == 0:
        den //= 5
        m += 1
    if den != 1:
        return None
    e = max(n, m)
    if n >= m:
        if num % (2 ** n) == 0:
            num //= 2 ** n
    elif num % (5 ** m) == 0:
        num //= 5 ** m
    s = str(num)
    # trailing zeros are not significant digits (the C counts them out; see fmt_dec)
    while len(s) > 1 and s[-1] == '0':
        s = s[:-1]
    return len(s)


def fmt_dec(y):
    """fmt of conventions 9.5: an exact decimal, positional for -4 <= X <= 20."""
    y = _fr(y)
    if y == 0:
        return "0"
    neg = y < 0
    y = abs(y)
    num, den = y.numerator, y.denominator
    a = b = 0
    while den % 2 == 0:
        den //= 2
        a += 1
    while den % 5 == 0:
        den //= 5
        b += 1
    assert den == 1, y
    if a >= b:
        num *= 5 ** (a - b)
        e = -a
    else:
        num *= 2 ** (b - a)
        e = -b
    s = str(num)
    while len(s) > 1 and s[-1] == '0':
        s = s[:-1]
        e += 1
    k = len(s)
    xx = e + k - 1
    if -4 <= xx <= 20:
        if e >= 0:
            body = s + "0" * e
        elif k + e > 0:
            body = s[:k + e] + "." + s[k + e:]
        else:
            body = "0." + "0" * (-(k + e)) + s
    else:
        body = s[0] if k == 1 else s[0] + "." + s[1:]
        body = "%se%d" % (body, xx)
    return ("-" + body) if neg else body


def print_real(mid, rad, n=20, k=2):
    """Steps 1 to 6 of conventions 9.5 with k = 2 (the unconstrained algorithm)."""
    if rad == 0:
        d = sig_digits(mid)
        if d is not None and d <= n:
            return fmt_dec(mid)
    if mid == 0:
        return "0 +/- %s" % fmt_dec(ceil2(rad))
    q = X(mid) - n + 1
    if rad > 0:
        q = max(q, X(rad) - 1)
    for _ in range(200):
        M = round_q(mid, q)
        R = ceil2(rad + abs(M - mid))
        q2 = X(R) - 1 if M == 0 else max(X(M) - n + 1, X(R) - 1)
        mult = F(10) ** q2 if q2 >= 0 else F(1, 10 ** (-q2))
        if M % mult == 0:
            return "%s +/- %s" % (fmt_dec(M), fmt_dec(R))
        if q2 == q:
            return "%s +/- %s" % (fmt_dec(M), fmt_dec(R))
        q = q2
    raise AssertionError("print_real did not end")


def print_lift(mid, rad, fin, n=20):
    return "(%s ; %s) + Q" % (print_real(mid, rad, n), fin_text(fin))


# ------------------------------------------------------------------ the C harness

class C:
    def __init__(self, exe):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1)

    def cmd(self, s):
        self.p.stdin.write(s + "\n")
        self.p.stdin.flush()
        return self.p.stdout.readline().rstrip("\n")

    def read(self, text, prec=64, ml=-999, me=-1, mp=-1, mi=-1):
        line = self.cmd("READ %d %d %d %d %d %s" % (prec, ml, me, mp, mi, text))
        f = line.split()
        out = {'status': f[1], 'changed': int(f[2]), 'canon': int(f[3])}
        out['printed'] = None if f[4] == '-' else bytes.fromhex(f[4]).decode()
        if out['status'] == 'OK':
            out['mid'] = F(int(f[5]), int(f[6]))
            out['rad'] = F(int(f[7]), int(f[8]))
        return out

    def set_ref(self, mm, me, rm, re, A, H, d):
        self.cmd("SET %s %d %s %d %d %d %d" % (mm, me, rm, re, A, H, d))

    def close(self):
        self.p.stdin.close()
        self.p.wait()


# ------------------------------------------------------------------ the table

# (text, prec, limits, expected status, note)
TABLE = [
    ("(0.5 ; 0) + Q", 64, {}, "OK", "the smallest lift"),
    ("(0;0)+Q", 64, {}, "OK", "no spaces"),
    ("  (0.5 ; 0) + Q  ", 64, {}, "OK", "whitespace around"),
    ("( 0.5 ; 0 ) + Q", 64, {}, "OK", "whitespace between tokens"),
    ("(-0.5 ; 0) + Q", 64, {}, "OK", "negative real, no range condition on a lift"),
    ("(5 ; 0) + Q", 64, {}, "OK", "outside [0,1]: CV-45 binds the pieces, not a lift"),
    ("(0.5 ; 0) + q", 64, {}, "PARSE", "keywords are case-sensitive (8.2)"),
    ("(0.5 ; 0) + QX", 64, {}, "PARSE", "Q must be the whole letter run (8.3)"),
    ("(0.5 ; 0)", 64, {}, "PARSE", "the marker `+ Q` is required (9.2)"),
    ("(0.5 ; 0) + Q + Q", 64, {}, "PARSE", "the whole input must be consumed (9.2)"),
    ("(0.5 ; 0) + QQ", 64, {}, "PARSE", "letter run QQ"),
    ("(0.5 ; 0)++Q", 64, {}, "PARSE", "two plus signs"),
    ("((0.5 ; 0)) + Q", 64, {}, "PARSE", "no nested parentheses in the grammar"),
    ("(0.5 ; 0) + Q ", 64, {}, "OK", "trailing whitespace"),
    ("\t(0.5 ; 0) + Q", 64, {}, "OK", "leading TAB"),
    ("(0.5\n;\t0) + Q", 64, {}, "OK", "newline and TAB inside"),
    ("(0.5 ; 0) + Q\x00", 64, {}, "PARSE", "NUL byte (8.2, 8.1)"),
    ("(0.5\x80 ; 0) + Q", 64, {}, "PARSE", "byte >= 0x80 (8.2)"),
    ("(0.5\x01 ; 0) + Q", 64, {}, "PARSE", "control byte (8.2)"),
    ("(0.5 ; 0) + Q", 64, {'ml': 5}, "LIMIT", "stage 1: len > max_len, before any byte (8.5)"),
    ("(0.5\x80 ; 0) + Q", 64, {'ml': 5}, "LIMIT", "stage 1 before stage 2 (8.5)"),
    ("(1/0 ; 0) + Q", 64, {}, "PARSE", "`/` is not in the grammar of a real (9.1)"),
    ("(0.5 ; 1/0) + Q", 64, {}, "DOMAIN", "zero denominator of the centre (9.3)"),
    ("(0.5 ; 1 mod 0) + Q", 64, {}, "OK", "`mod 0` is dropped (9.3)"),
    ("(0.5 ; 2 mod 4) + Q", 64, {}, "OK", "the centre is reduced into [0,N) (9.3)"),
    ("(0.5 ; -3) + Q", 64, {}, "OK", "an exact finite point may be negative"),
    ("(0.5 ; 007) + Q", 64, {}, "OK", "leading zeros are removed (9.3)"),
    ("(0.5 ; 6/4) + Q", 64, {}, "OK", "fractions are reduced (9.3)"),
    ("(0.5 ; -0) + Q", 64, {}, "OK", "-0 becomes 0 (9.3)"),
    ("(0.500 ; 0) + Q", 64, {}, "OK", "decimals are rewritten by 9.5"),
    ("(1.5 +/- 1e-5 ; 5/3 mod 6) + Q", 64, {}, "OK", "the example of 9.4"),
    ("(1e100000 ; 0) + Q", 64, {}, "OK", "the exponent is at max_exp10 (8.4)"),
    ("(1e100001 ; 0) + Q", 64, {}, "LIMIT", "stage 4: abs(exponent) > max_exp10 (8.4)"),
    ("(1e-100001 ; 0) + Q", 64, {}, "LIMIT", "stage 4, negative"),
    ("(0e100001 ; 0) + Q", 64, {}, "LIMIT", "a zero coefficient does not excuse it (8.4)"),
    ("(1e9999999999999999999 ; 0) + Q", 64, {}, "LIMIT", "a 19-digit exponent (8.4)"),
    ("(1e0000000000000000005 ; 0) + Q", 64, {}, "OK", "leading zeros are skipped (8.4)"),
    ("(1e9999999 ; 0) + Q", 64, {'me': 10000000}, "OK", "max_exp10 = 10^7"),
    ("(1e9999999 ; 0) + Q", 64, {'me': 1000000}, "LIMIT", "7 digits, over 10^6"),
    ("(0.5 ; 0) + Q", 2097153, {}, "LIMIT", "prec above ADF_REAL_PREC_MAX comes first (api-3.md 1)"),
    ("(0.5\x80 ; 0) + Q", 2097153, {}, "LIMIT", "the precision limit precedes the byte check"),
    ("(0.5 ; 0) + Q", 2, {}, "OK", "prec below 2 is taken as 2 (8.1)"),
    ("(0.5 ; 0) + Q", -5, {}, "OK", "a negative prec is taken as 2 (8.1)"),
    ("(0.5 ; 0) + Q", 0, {}, "OK", "prec 0 is taken as 2 (8.1)"),
    ("(.5 ; 0) + Q", 64, {}, "PARSE", "no `.5` in the grammar (9.1)"),
    ("(5. ; 0) + Q", 64, {}, "PARSE", "no `5.` in the grammar (9.1)"),
    ("(+0.5 ; 0) + Q", 64, {}, "PARSE", "no `+` sign on a number (9.1)"),
    ("(0.5e ; 0) + Q", 64, {}, "PARSE", "an exponent needs digits (9.1)"),
    ("(0.5 ; 0 ) + Q extra", 64, {}, "PARSE", "trailing token"),
    ("(0.5 ; ) + Q", 64, {}, "PARSE", "the finite part is required"),
    ("(0.5 ; 0", 64, {}, "PARSE", "the closing parenthesis is required"),
    ("( ; 0) + Q", 64, {}, "PARSE", "the real part is required"),
    # the union form: refused, but only after the checks of 8.5
    ("union((0.5 ; 0)) + Q", 64, {}, "UNSUPPORTED", "one piece, syntax fine"),
    ("union((0.5 ; 0),(0.25 ; 1 mod 3)) + Q", 64, {}, "UNSUPPORTED", "two pieces"),
    ("union ( (0.5 ; 0) ) + Q", 64, {}, "UNSUPPORTED", "whitespace is allowed (8.2)"),
    ("UNION((0.5 ; 0)) + Q", 64, {}, "PARSE", "case-sensitive keyword"),
    ("union() + Q", 64, {}, "PARSE", "the grammar needs one adele (9.2)"),
    ("union((0.5 ; 0),) + Q", 64, {}, "PARSE", "a trailing comma"),
    ("union((0.5 ; 0) + Q) + Q", 64, {}, "PARSE", "a nested marker"),
    ("union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q", 64, {'mi': 2}, "LIMIT",
     "stage 4 count, before the refusal"),
    ("union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q", 64, {'mi': 3}, "UNSUPPORTED",
     "the count is not above the limit"),
    ("union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q", 64, {'mi': 0}, "LIMIT", "max_items = 0"),
    ("union((0.5 ; 0),) + Q", 64, {'mi': 1}, "PARSE", "stage 3 before stage 4"),
    ("union((1e100001 ; 0)) + Q", 64, {}, "LIMIT", "stage 4 before the refusal"),
    ("union((0.5 ; 0) + Q", 64, {}, "PARSE", "malformed union"),
    ("union((0.5 ; 0)) + Q", 64, {'ml': 5}, "LIMIT", "stage 1 before stage 3"),
    ("union((0.5 ; 0)) + Q", 64, {'me': 1}, "UNSUPPORTED", "no decimal in this text"),
    ("union((0.5 ; 1/0)) + Q", 64, {}, "UNSUPPORTED",
     "stage 6 is value semantics, after the refusal"),

    ('union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q', 64, {'mi': 2}, "LIMIT",
     "stage 4 count, before the refusal"),
    ('union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q', 64, {'mi': 3}, "UNSUPPORTED",
     "the count is not above the limit"),
    ('union((0.5 ; 0),(0.5 ; 0),(0.5 ; 0)) + Q', 64, {'mi': 0}, "LIMIT", "max_items = 0"),
    ('(0.5 ; 0) + Q', 64, {'ml': 1048576, 'me': 100000, 'mp': 100000, 'mi': 1048576}, "OK",
     "the defaults of 8.4 written out"),
]


def random_lifts(c, seed, n):
    """Valid texts of random lifts: status, canonicality, enclosure, print, reread."""
    import random
    rng = random.Random(seed)
    bad = 0
    for i in range(n):
        ip = rng.randrange(0, 10 ** rng.randrange(0, 12))
        fp = rng.randrange(0, 10 ** rng.randrange(0, 8))
        m = "%d.%d" % (ip, fp) if fp else "%d" % ip
        if rng.random() < 0.3:
            m = "-" + m
        if rng.random() < 0.4:
            m += "e" + rng.choice(["", "+", "-"]) + str(rng.randrange(0, 30))
        r = ""
        if rng.random() < 0.6:
            r = " +/- %d.%de-%d" % (rng.randrange(0, 10 ** 6), rng.randrange(0, 10 ** 6),
                                    rng.randrange(0, 12))
        a = rng.randrange(-50, 50)
        den = rng.choice([1, 1, 2, 3, 7])
        if rng.random() < 0.5:
            fin = "%d/%d mod %d" % (a, den, rng.randrange(0, 40))
        else:
            fin = "%d" % (a if rng.random() < 0.5 else a // den)
        text = "(%s%s ; %s) + Q" % (m, r, fin)
        prec = rng.choice([2, 24, 64, 128, 333])
        c.set_ref("3", 0, "0", 0, 7, 0, 1)
        got = c.read(text.encode().hex(), prec)
        if got['status'] != 'OK':
            bad += 1
            print("RANDOM NOT OK %r prec %d -> %s" % (text, prec, got['status']))
            continue
        if got['canon'] != 1 or got['changed'] != 1:
            bad += 1
            print("RANDOM NOT CANONICAL/UNCHANGED %r" % text)
        r2 = Reader(text)
        (mm, rd), fn = r2.adele()
        r2.lit('+')
        r2.kw('Q')
        assert r2.at_end()
        if not (mm - rd >= got['mid'] - got['rad'] and mm + rd <= got['mid'] + got['rad']):
            bad += 1
            print("RANDOM NOT ENCLOSED %r" % text)
        big = (got['mid'] != 0 and bitlen2(got['mid']) > 100000) or \
              (got['rad'] != 0 and bitlen2(got['rad']) > 100000)
        if got['printed'] is None:
            if not big:
                bad += 1
                print("RANDOM REFUSED %r" % text)
            continue
        mine = print_lift(got['mid'], got['rad'], fn)
        if got['printed'] != mine:
            bad += 1
            print("RANDOM PRINT DIFFERS %r\n   C    %r\n   mine %r" % (text, got['printed'], mine))
        r3 = Reader(got['printed'])
        (pm, pr), pfn = r3.adele()
        if not (pm - pr <= got['mid'] - got['rad'] and pm + pr >= got['mid'] + got['rad']):
            bad += 1
            print("RANDOM REREAD NOT CONTAINING %r -> %r" % (text, got['printed']))
    print("random lifts %d" % n)
    return bad


def my_status(text, lim):
    """My own reading of the text through stages 1 to 4 of 8.5 (the C refuses unions at stage 6)."""
    data = text.encode('utf-8', 'surrogateescape')
    if lim.get('ml', 1048576) >= 0 and len(data) > lim['ml']:
        return 'LIMIT'
    for c in data:
        if not (0x20 <= c <= 0x7E or c in (0x09, 0x0A, 0x0D)):
            return 'PARSE'
    try:
        r = Reader(text)
        form = r.qclass()
    except Bad as e:
        return e.status
    if form[0] == 'union':
        mi = lim.get('mi', 1048576)
        if mi < 1 or form[1] > mi:
            return 'LIMIT'
        return 'UNSUPPORTED'
    # stage 4: the exponents of the decimals (the lift has one real)
    return 'OK'


def check_exponents(text):
    """Stage 4 of the lift form: every decimal exponent against max_exp10."""
    for m in re.finditer(DEC, text):
        d = dec_exponent_digits(m.group(0))
        if d is not None and len(d) > 18:
            return True
    return False


def main():
    exe = os.environ.get('TEXT_EVAL', './lanes/q-review2/text_eval')
    c = C(exe)
    bad = 0
    seen = set()
    for text, prec, lim, want, note in TABLE:
        data = text.encode('utf-8', 'surrogateescape')
        c.set_ref("3", 0, "0", 0, 7, 0, 1)          # a reference value no text produces
        ml = lim.get('ml', None)
        if ml is None:
            ml = 1048576 if any(k in lim for k in ('me', 'mp', 'mi')) else -999
        # the fields of adf_text_limits_t are counts and exponents: pass the defaults of 8.4,
        # never a negative value (that is an invalid limit, conventions 4.4)
        got = c.read(data.hex(), prec, ml, lim.get('me', 100000), lim.get('mp', 100000),
                     lim.get('mi', 1048576))
        # proto/text_grammar.py is not run on texts with a decimal exponent above 1000: its
        # _decimal_parts (proto/text_grammar.py:132) loops over 10^(2k) and does not finish.
        skip_ref = any(abs(int(d.lstrip('0') or '0')) > 1000
                       for d in [dec_exponent_digits(m.group(0))
                                 for m in re.finditer(DEC, text)] if d)
        if skip_ref:
            ref_status = 'SKIPPED'
            ref = None
        else:
            ref = tg.canonical('qclass', data, tg.Limits(
                max_len=lim.get('ml', 1048576), max_exp10=lim.get('me', 100000),
                max_prec=lim.get('mp', 100000), max_items=lim.get('mi', 1048576)))
            ref_status = ref[1:] if ref.startswith('!') else 'OK'
        seen.add((got['status'], ref_status))
        if got['status'] != want:
            bad += 1
            print("STATUS %-38r want %-13s C %-13s ref %-13s %s" % (text, want, got['status'],
                                                                     ref_status, note))
        if got['status'] == 'OK':
            if got['canon'] != 1:
                bad += 1
                print("NOT CANONICAL on OK %r" % text)
            if got['changed'] != 1:
                bad += 1
                print("UNCHANGED on OK %r" % text)
            # my own reading of the text, exact interval
            r2 = Reader(text)
            rr = r2.adele()
            (mm, rd), fn = rr
            r2.lit('+')
            r2.kw('Q')
            assert r2.at_end()
            exact = (mm - rd, mm + rd)
            if not (exact[0] >= got['mid'] - got['rad'] and exact[1] <= got['mid'] + got['rad']):
                bad += 1
                print("C BALL DOES NOT ENCLOSE THE TEXT %r: %s vs [%s,%s]" % (
                    text, got['mid'], exact[0], exact[1]))
            big = (got['mid'] != 0 and bitlen2(got['mid']) > 100000) or \
                  (got['rad'] != 0 and bitlen2(got['rad']) > 100000)
            if got['printed'] is None:
                if not big:
                    bad += 1
                    print("PRINTER REFUSED a printable value %r" % text)
                continue
            if big:
                bad += 1
                print("PRINTER DID NOT REFUSE %r (binary exponent over ADF_PRINT_EXP_MAX)" % text)
            mine = print_lift(got['mid'], got['rad'], fn)
            if got['printed'] != mine:
                bad += 1
                print("PRINT DIFFERS %r\n   C      %r\n   mine   %r\n   stored %s +/- %s" % (
                    text, got['printed'], mine, got['mid'], got['rad']))
            if ref_status == 'OK' and ref != got['printed']:
                bad += 1
                print("REF DIFFERS %r\n   C      %r\n   ref    %r" % (text, got['printed'], ref))
            # the printed text read again contains the stored ball (9.6)
            r3 = Reader(got['printed'])
            (pm, pr), pfn = r3.adele()
            if not (pm - pr <= got['mid'] - got['rad'] and pm + pr >= got['mid'] + got['rad']):
                bad += 1
                print("REREAD DOES NOT CONTAIN %r: %s" % (text, got['printed']))
        else:
            if got['changed'] != 0:
                bad += 1
                print("OUTPUT CHANGED on %s for %r" % (got['status'], text))
    # every proper prefix and suffix of a valid lift is a parse error
    full = "(1.5 +/- 1e-5 ; 5/3 mod 6) + Q"
    for k in range(0, len(full)):
        for cut in (full[:k], full[k:]):
            c.set_ref("3", 0, "0", 0, 7, 0, 1)
            got = c.read(cut.encode().hex(), 64, -999, 100000, 100000, 1048576)
            want = 'OK' if cut == full else 'PARSE'
            if got['status'] != want:
                bad += 1
                print("CUT %r -> C %s want %s" % (cut, got['status'], want))
    bad += random_lifts(c, int(os.environ.get('SEED', '5')), int(os.environ.get('NRAND', '500')))
    print("pairs (C, reference) seen:", sorted(seen))
    c.close()
    print("problems %d" % bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
