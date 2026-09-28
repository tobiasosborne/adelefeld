"""Independent Fraction oracle; the adapter only extracts FLINT's stored dyadic data."""
import ctypes as C
import json
import random
import sys
import time
from fractions import Fraction as F
from pathlib import Path

sys.dont_write_bytecode = True
sys.set_int_max_str_digits(0)
HERE = Path(__file__).resolve().parent
lib = C.CDLL(str(HERE / "bridge.so"))


class Limits(C.Structure):
    _fields_ = [("max_len", C.c_size_t), ("max_exp10", C.c_long),
                ("max_prec", C.c_long), ("max_items", C.c_long)]


lib.adf_str_free.argtypes = [C.c_void_p]
lib.review_parse.argtypes = [C.c_char_p, C.c_size_t, C.c_long, C.POINTER(Limits), C.c_int,
                            C.POINTER(C.c_void_p)]
lib.review_status.argtypes = [C.c_int, C.c_char_p, C.c_size_t, C.POINTER(Limits)]
lib.adf_text_classify.argtypes = [C.POINTER(C.c_int), C.c_char_p, C.c_size_t, C.POINTER(Limits)]
lib.review_print.argtypes = [C.c_char_p, C.c_long, C.c_ulong, C.c_long, C.c_long]
lib.review_print.restype = C.c_void_p


def take(ptr):
    text = C.string_at(ptr).decode()
    lib.adf_str_free(ptr)
    return text


def dyad(m, e):
    return F(m << e) if e >= 0 else F(m, 1 << -e)


def parse(text, p, part=-1, lim=None):
    data = text.encode() if isinstance(text, str) else text
    ptr = C.c_void_p()
    st = lib.review_parse(data, len(data), p, C.byref(lim) if lim else None, part, C.byref(ptr))
    if st:
        return st, None
    m, e, r, re = map(int, take(ptr).split())
    return st, (dyad(m, e), dyad(r, re))


def bits(x):
    n, d = abs(x.numerator), x.denominator
    if d & (d - 1):
        return 10**9
    return (n // (n & -n)).bit_length() if n else 0


def encloses(outer, inner):
    a, b = outer
    m, r = inner
    return b >= r + abs(a - m)


def decimal(rng, signed=True):
    n = rng.randrange(1, 81)
    digits = ''.join(str(rng.randrange(10)) for _ in range(n))
    if rng.randrange(2):
        at = rng.randrange(1, n + 1)
        digits = digits[:at] + '.' + (digits[at:] or '0')
    e = rng.choice([rng.randrange(-500, 501), 0, 0, -1, 1])
    return ('-' if signed and rng.randrange(2) else '') + digits + rng.choice(['e', 'E']) + f'{e:+}'


def enclosure():
    start = time.monotonic()
    rng = random.Random(280926)
    count = exact = complex_count = 0

    def check(mt, rt, p, complex_part=-1):
        nonlocal count, exact, complex_count
        text = f'({mt} +/- {rt} ; 7/3 mod 12/5)'
        if complex_part >= 0:
            text = f'(({mt} +/- {rt}) + ({mt} +/- {rt})*i ; 7/3 mod 12/5)'
            complex_count += 1
        wanted = F(mt), F(rt)
        st, got = parse(text, p, complex_part)
        assert st == 0 and encloses(got, wanted), (text, p, st, got)
        assert bits(got[0]) <= p, (text, p, got)
        if bits(wanted[0]) <= p and bits(wanted[1]) <= 30:
            assert got == wanted, (text, p, got, wanted)
            exact += 1
        count += 1

    for i in range(100000):
        p = 2 + i % 255 if i % 3 else rng.choice([2, 3, 29, 30, 31, 53, 64, 128, 1024])
        mt = decimal(rng)
        rt = '0' if i % 7 == 0 else decimal(rng, False)
        check(mt, rt, p, (i // 10) % 2 if i % 10 == 0 else -1)
    random_count = count
    for p in range(2, 257):
        for m in [(1 << p) + 1, (1 << p) + 3, -((1 << p) + 1), (1 << p) - 1]:
            for rt in ['0', '0.125', '1073741823', '1073741825', '1e-100', '1e100']:
                check(str(m), rt, p)
    halfway_count = count - random_count
    before = count
    for p in [2, 3, 30, 53, 128, 1024]:
        for mt, rt in [('1e100000', '0'), ('-1e-100000', '0'), ('0', '1e100000'),
                       ('1e100000', '1e-100000'), ('1e-100000', '1e100000'),
                       ('0.1e-100000', '1e-100000')]:
            check(mt, rt, p)
    print(json.dumps(dict(random_texts=random_count, halfway_cases=halfway_count,
                          exponent_boundary_cases=count-before, total=count, exact_cases=exact,
                          complex_cases=complex_count, failures=0, seconds=round(time.monotonic()-start, 3))))


def printing():
    start = time.monotonic()
    rng = random.Random(280927)
    count = 0

    def check(m, e, r, re, digits, p):
        nonlocal count
        text = take(lib.review_print(str(m).encode(), e, r, re, digits))
        value = dyad(m, e), dyad(r, re)
        real = text[1:text.index(' ;')]
        a, _, b = real.partition(' +/- ')
        printed = F(a), F(b or '0')
        assert encloses(printed, value), (m, e, r, re, digits, text)
        st, reread = parse(text, p)
        assert st == 0 and encloses(reread, value), (m, e, r, re, digits, p, text)
        count += 1

    for i in range(12000):
        m = rng.randrange(-(1 << rng.randrange(1, 257)), 1 << rng.randrange(1, 257))
        e, re = rng.randrange(-1000, 1001), rng.randrange(-1000, 1001)
        r = 0 if i % 7 == 0 else rng.randrange(1 << 30)
        digits = rng.choice([1, 2, 3, 20, 53, 128, 256, 999999, 1000000])
        check(m, e, r, re, digits, 2 + i % 255)
    # Every allowed digits value; exact zero exercises the entire public parameter domain cheaply.
    for digits in range(1, 1000001):
        text = take(lib.review_print(b'0', 0, 0, 0, digits))
        assert text == '(0 ; 0)', (digits, text)
    print(json.dumps(dict(nonzero_cases=count, zero_cases=1000000, failures=0,
                          seconds=round(time.monotonic()-start, 3))))


if __name__ == '__main__':
    {'enclosure': enclosure, 'printing': printing}[sys.argv[1]]()
