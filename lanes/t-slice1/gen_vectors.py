#!/usr/bin/env python3
"""lanes/t-slice1/gen_vectors.py: vectors for the C value form of adf_ucoset, adf_idele, adf_idclass.

The oracle is the reference proto/text_grammar.py (docs/conventions.md 8, 9); nothing here computes an expected
value by itself, except the gap of an interval, which is plain arithmetic on exact rationals. Written to
tests/ref/vectors/t-slice1/:

  golden_parts.jsonl  for every non-status row of tests/golden/{ucoset,idele,idclass}.tsv: the exact decimal
                      midpoint and radius of the real part, the reduced content r, and the stored unit (c, N)
  text_idele.jsonl    about 2000 random texts of the three kinds (canonical, non-canonical, mutated into faults):
                      the reference status or canonical text, the exact parts, the stored unit, the gap

The gap of a valid idele or class: for the absolute interval [lo, hi] of the real part (lo > 0), gap = e(hi) - e(lo)
with e(t) = floor(log2 t) + 1. The C reader may answer NOT_DETERMINED only if gap >= p (p the working
precision; the kernel B condition e(hi') - e(lo') > p with hi' = RU_p(hi), lo' = RD_p(lo) has e(lo') = e(lo)
and e(hi') <= e(hi) + 1); it must answer OK if gap <= p - 1.

Inputs are stored as hexadecimal, so that any byte (NUL, bytes >= 0x80) survives the JSON reader.

    python3 lanes/t-slice1/gen_vectors.py          (from the repository root)
"""
import json
import os
import random
import sys
from fractions import Fraction

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "proto"))
sys.path.insert(0, os.path.join(ROOT, "lanes", "m1-text"))
import text_grammar as tg  # noqa: E402
import gen_text_vectors as g  # noqa: E402  (its helpers: unescape, golden_rows, dec_parts, real_parts_of)

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "t-slice1")
SEED = 20260930


def e_of(t):
    """e(t) = floor(log2 t) + 1 for a rational t > 0."""
    t = Fraction(t)
    e0 = t.numerator.bit_length() - t.denominator.bit_length()
    fl = e0 if t >= Fraction(2) ** e0 else e0 - 1
    return fl + 1


def unit_of(node):
    c, N = tg._ucoset(node)
    return {"c": str(c), "N": str(N)}


def gap_of(rp):
    mid, rad = tg._real(rp)
    lo, hi = mid - rad, mid + rad
    if lo > 0:
        a, b = lo, hi
    else:
        a, b = -hi, -lo
    assert a > 0
    return e_of(b) - e_of(a)


def info(kind, data):
    """The parts of a text that the reference reads: a dict, or None if the reference gives a status."""
    s = tg._prep(data, tg.DEFAULT_LIMITS)
    node = tg._syntax(kind, s)
    if kind == "ucoset":
        return unit_of(node[1])
    if kind == "idele":
        _, rn, scale, un = node
        r = tg._rat(scale)
        d = {"parts": [g.real_parts_of(rn)], "r": "%d/%d" % (r.numerator, r.denominator), "gap": gap_of(rn)}
        d.update(unit_of(un))
        return d
    _, rn, un = node
    d = {"parts": [g.real_parts_of(rn)], "gap": gap_of(rn)}
    d.update(unit_of(un))
    return d


def record(kind, data, extra=None):
    exp = tg.canonical(kind, data)
    rec = {"kind": kind, "hex": data.hex(), "expected": exp, "cls": tg.classify(data)}
    if not exp.startswith("!"):
        rec.update(info(kind, data))
    if extra:
        rec.update(extra)
    return rec


def write(name, records):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w", encoding="ascii") as fh:
        for r in records:
            fh.write(json.dumps(r, sort_keys=True) + "\n")
    print("%s: %d records" % (os.path.relpath(path, ROOT), len(records)))


def gen_golden():
    recs = []
    for fname in ("ucoset", "idele", "idclass"):
        for line, data, exp, gen in g.golden_rows(fname):
            if gen:
                continue
            r = record(fname, data, {"file": fname, "line": line})
            # the golden expectation and the reference agree (the README of tests/golden says so)
            assert r["expected"] == exp, (fname, line, r["expected"], exp)
            recs.append(r)
    write("golden_parts.jsonl", recs)


# ------------------------------------------------------------------------------------------ random texts

def ws(rng):
    r = rng.random()
    if r < 0.6:
        return ""
    if r < 0.85:
        return " "
    return "".join(rng.choice(" \t\r\n") for _ in range(rng.randrange(1, 4)))


def zeros(rng):
    return "0" * rng.choice([0, 0, 0, 0, 1, 3])


def r_uint(rng, big=False):
    if big and rng.random() < 0.15:
        return str(rng.randrange(1, 10 ** rng.randrange(20, 300)))
    return str(rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 18, 24, 30, 36, 100, 210, rng.randrange(10 ** 6)]))


def r_urat(rng, big=False):
    s = zeros(rng) + r_uint(rng, big)
    if rng.random() < 0.5:
        s += "/" + zeros(rng) + r_uint(rng, big)
    return s


def r_ucoset(rng):
    c = rng.choice(["", "", "-"]) + zeros(rng) + r_uint(rng, True)
    s = "[" + ws(rng) + c
    if rng.random() < 0.85:
        s += ws(rng) + (" " if rng.random() < 0.9 else "") + "mod" + ws(rng) + " " + zeros(rng) + r_uint(rng, True)
    return s + ws(rng) + "]"


def r_unit(rng):
    if rng.random() < 0.5:
        return r_ucoset(rng)
    from math import gcd
    N = rng.choice([1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 18, 30, 36, 60, 100, 2 * rng.randrange(1, 10 ** 6) + 2,
                    rng.randrange(1, 10 ** 30)])
    while True:
        c = rng.randrange(-3 * N, 3 * N)
        if gcd(c, N) == 1:
            break
    return "[" + ws(rng) + str(c) + ws(rng) + " mod" + ws(rng) + " " + zeros(rng) + str(N) + ws(rng) + "]"


def r_udec_dyadic(rng):
    """A decimal literal whose value is dyadic with a small odd mantissa."""
    m = rng.randrange(0, 1 << 20)
    k = rng.randrange(0, 12)
    val = Fraction(m, 2 ** k)
    if val == 0:
        return "0"
    D, E = g.dec_parts(val)
    style = rng.randrange(3)
    if style == 0 or E >= 0:
        if E >= 0:
            return str(D) + ("0" * E if E < 30 else "e%d" % E)
        return str(D) + "e%d" % E
    ds = str(D)
    if len(ds) > -E:
        return ds[:len(ds) + E] + "." + ds[len(ds) + E:]
    return "0." + "0" * (-E - len(ds)) + ds


def r_udec_any(rng):
    s = zeros(rng) + str(rng.randrange(0, 10 ** rng.randrange(1, 25)))
    if rng.random() < 0.5:
        s += "." + str(rng.randrange(0, 10 ** rng.randrange(1, 25))).rjust(rng.randrange(1, 4), "0")
    if rng.random() < 0.3:
        s += rng.choice("eE") + rng.choice(["", "+", "-"]) + str(rng.randrange(0, 40))
    return s


def r_real_plain(rng):
    gen = r_udec_dyadic if rng.random() < 0.5 else r_udec_any
    s = rng.choice(["", "", "-"]) + gen(rng)
    if rng.random() < 0.6:
        s += ws(rng) + "+/-" + ws(rng) + (r_udec_dyadic(rng) if rng.random() < 0.5 else r_udec_any(rng))
    return s


def r_real_margin(rng, sign):
    """m +/- r with r just inside m (a margin of a few digits), so that the sign condition is close to failing:
    exercises the second ball of the reader (the kernel) and the statuses NOT_DETERMINED at a small prec."""
    m = rng.choice([1, 2, 3, 5, 7, 10, 12345])
    nines = rng.randrange(1, 45)
    r = "0." + "9" * nines
    if rng.random() < 0.3:
        r = str(m - 1) + "." + "9" * nines if m > 1 else r
    s = ("-" if sign < 0 else "") + str(m) + ws(rng) + "+/-" + ws(rng) + r
    return s


def r_real_pos(rng):
    """A positive real ball: mostly valid."""
    r = rng.random()
    if r < 0.35:
        return r_real_margin(rng, 1)
    return r_real_plain(rng)


def r_text(rng, kind):
    if kind == "ucoset":
        return ws(rng) + r_unit(rng) + ws(rng)
    if kind == "idele":
        real = r_real_margin(rng, rng.choice([1, -1])) if rng.random() < 0.3 else r_real_plain(rng)
        return ("(" + ws(rng) + real + ws(rng) + ";" + ws(rng) + r_urat(rng, True) + ws(rng) + "*" + ws(rng)
                + r_unit(rng) + ws(rng) + ")")
    return ("<" + ws(rng) + r_real_pos(rng) + ws(rng) + ";" + ws(rng) + r_unit(rng) + ws(rng) + ">")


ALPHABET = b" ()[]<>{};,:*+=^/-.eE0123456789modpORCiQunfftrchaDMPABqns\x00\x80\xff\t\x7f"


def mutate(rng, data):
    b = bytearray(data)
    for _ in range(rng.randrange(1, 4)):
        op = rng.randrange(3)
        pos = rng.randrange(len(b) + 1)
        if op == 0 or not b:
            b[pos:pos] = bytes([rng.choice(ALPHABET)])
        elif op == 1:
            del b[min(pos, len(b) - 1)]
        else:
            b[min(pos, len(b) - 1)] = rng.choice(ALPHABET)
    return bytes(b)


def gen_texts(rng):
    recs = []
    seen = set()
    want = {"ucoset": 500, "idele": 900, "idclass": 800}
    for kind, n in want.items():
        k = 0
        while k < n:
            data = r_text(rng, kind).encode("ascii")
            if rng.random() < 0.3:
                data = mutate(rng, data)
            if (kind, data) in seen:
                continue
            seen.add((kind, data))
            recs.append(record(kind, data))
            k += 1
    write("text_idele.jsonl", recs)
    ok = {k: sum(1 for r in recs if r["kind"] == k and not r["expected"].startswith("!")) for k in want}
    print("valid records:", ok)


def gen_print(rng):
    """Dyadic balls (mid = mm 2^me, rad = rm 2^re, rm odd below 2^30) that satisfy a sign condition, the digits n,
    and the constrained printing of docs/conventions.md 9.5 (proto print_real with cond): the real part of the
    printed idele (cond nonzero) or class (cond positive)."""
    recs = []
    for i in range(1500):
        cond = rng.choice(["nonzero", "positive"])
        mbits = rng.choice([1, 2, 5, 20, 40, 53, 64, 100])
        mm = rng.randrange(1 << (mbits - 1), 1 << mbits) | 1
        if cond == "nonzero" and rng.random() < 0.5:
            mm = -mm
        me = rng.randrange(-60, 60) if i % 7 else rng.randrange(-900, 900)
        mid = Fraction(mm) * Fraction(2) ** me
        if rng.random() < 0.15:
            rm, re_ = 0, 0
        else:
            f = rng.choice([Fraction(1, 10), Fraction(1, 2), Fraction(9, 10), Fraction(99, 100),
                            Fraction(999999, 1000000), Fraction(1 << 20, (1 << 20) + 1), Fraction(1)
                            - Fraction(1, 1 << rng.randrange(2, 30))])
            re_ = None
            for _ in range(20):
                target = abs(mid) * f
                # the exponent that puts the mantissa of the radius below 2^30
                e = target.numerator.bit_length() - target.denominator.bit_length() - 28
                rm = int(target / Fraction(2) ** e)
                rm |= 1
                if 1 <= rm < (1 << 30) and Fraction(rm) * Fraction(2) ** e < abs(mid):
                    re_ = e
                    break
                f = f * Fraction(9, 10)
            if re_ is None:
                rm, re_ = 0, 0
        rad = Fraction(rm) * Fraction(2) ** re_
        assert abs(mid) > rad and (cond == "nonzero" or mid > 0)
        digits = rng.choice([1, 2, 3, 5, 10, 20, 20, 20, 30])
        text = tg.print_real(mid, rad, digits, cond)
        recs.append({"cond": cond, "mm": str(mm), "me": me, "rm": str(rm), "re": re_, "digits": digits,
                     "text": text})
    write("print_constrained.jsonl", recs)


def main():
    rng = random.Random(SEED)
    gen_golden()
    gen_texts(rng)
    gen_print(rng)


if __name__ == "__main__":
    main()
