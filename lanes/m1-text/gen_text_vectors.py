#!/usr/bin/env python3
"""lanes/m1-text/gen_text_vectors.py: vectors for the C value-form parser and printer (lane m1-text).

The oracle is the reference proto/text_grammar.py (docs/conventions.md 8, 9); nothing here computes an
expected value by itself. Written to tests/ref/vectors/m1-text/:

  real_parts.jsonl  for every non-status row of tests/golden/{adele,cadele,realball_read}.tsv: the exact
                    decimal midpoint and radius of each real ball of the input, as D * 10^E (the C test
                    decides the exactness condition of conventions 9.5 and 11.3 from them)
  print_real.jsonl  random dyadic balls (mid = mm 2^me, rad = rm 2^re) and digits -> print_real (9.5)
  text_rat.jsonl, text_fball.jsonl, text_adele.jsonl, text_cadele.jsonl
                    random texts (canonical, non-canonical, and mutated into faults) -> canonical(type, text),
                    with the real parts of a valid adele or cadele text as in real_parts.jsonl
  classify.jsonl    the inputs of every golden file of the thirteen types, of dispatch.tsv and of dump.tsv
                    (up to 4096 bytes), their mutations, and the random texts above -> classify(text)

Inputs are stored as hexadecimal, so that any byte (NUL, bytes >= 0x80) survives the JSON reader.

    python3 lanes/m1-text/gen_text_vectors.py          (from the repository root)
"""
import json
import os
import random
import sys
from fractions import Fraction

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "proto"))
import text_grammar as tg  # noqa: E402

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "m1-text")
SEED = 20260928


def unescape(field):
    """docs/conventions.md 11.1: \\\\ \\t \\n \\r \\xHH; @gen:PREFIX|UNIT|COUNT|SUFFIX."""
    def dec(t):
        out = bytearray()
        i = 0
        while i < len(t):
            c = t[i]
            if c == "\\":
                n = t[i + 1]
                if n == "\\":
                    out.append(0x5C); i += 2
                elif n == "t":
                    out.append(9); i += 2
                elif n == "n":
                    out.append(10); i += 2
                elif n == "r":
                    out.append(13); i += 2
                elif n == "x":
                    out.append(int(t[i + 2:i + 4], 16)); i += 4
                else:
                    raise ValueError(t)
            else:
                out.append(ord(c)); i += 1
        return bytes(out)
    if field.startswith("@gen:"):
        pre, unit, count, suf = field[5:].split("|")
        return dec(pre) + dec(unit) * int(count) + dec(suf)
    return dec(field)


def golden_rows(name):
    """(line, input bytes, expected, generated) of tests/golden/<name>.tsv; line numbers as the C reader."""
    rows = []
    with open(os.path.join(ROOT, "tests", "golden", name + ".tsv"), encoding="ascii") as fh:
        for no, line in enumerate(fh, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            inp, exp = line.split("\t")
            rows.append((no, unescape(inp), exp, inp.startswith("@gen:")))
    return rows


def dec_parts(x):
    """An exact decimal x as (D, E), x = D 10^E, D not divisible by 10 unless 0."""
    x = Fraction(x)
    if x == 0:
        return (0, 0)
    sign, D, E = tg._decimal_parts(x)
    return (sign * D, E)


def real_parts_of(node):
    mid, rad = tg._real(node)
    md, me = dec_parts(mid)
    rd, re_ = dec_parts(rad)
    return {"md": str(md), "me": me, "rd": str(rd), "re": re_}


def parts_of(type_name, data):
    """The real parts of a valid text of adele, cadele or a bare real ball, in order."""
    s = tg._prep(data, tg.DEFAULT_LIMITS)
    if type_name == "real":
        P = tg.Parser(s)
        node = P.real()
        return [real_parts_of(node)]
    node = tg._syntax(type_name, s)
    if type_name == "adele":
        return [real_parts_of(node[1])]
    z = node[1]
    return [real_parts_of(z[1]), real_parts_of(z[2])]


def write(name, records):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w", encoding="ascii") as fh:
        for r in records:
            fh.write(json.dumps(r, sort_keys=True) + "\n")
    print("%s: %d records" % (os.path.relpath(path, ROOT), len(records)))


# ------------------------------------------------------------------------------------------ real_parts

def gen_real_parts():
    recs = []
    for fname, tname in (("adele", "adele"), ("cadele", "cadele"), ("realball_read", "real")):
        for line, data, exp, gen in golden_rows(fname):
            if exp.startswith("!") or gen:
                continue
            recs.append({"file": fname, "line": line, "parts": parts_of(tname, data)})
    write("real_parts.jsonl", recs)


# ------------------------------------------------------------------------------------------ print_real

def gen_print_real(rng):
    recs = []
    digit_choices = [1, 2, 3, 4, 5, 10, 17, 20, 25, 40]
    for i in range(1500):
        mbits = rng.choice([0, 1, 2, 5, 20, 53, 64, 100, 128])
        mm = 0 if mbits == 0 else rng.randrange(1 << (mbits - 1), 1 << mbits) | 1
        if rng.random() < 0.5:
            mm = -mm
        me = rng.randrange(-200, 200) if i % 7 else rng.randrange(-1200, 1200)
        if rng.random() < 0.3:
            rm, re_ = 0, 0
        else:
            rm = rng.randrange(1, 1 << 30) | 1
            re_ = me + rng.randrange(-160, 40)
        digits = rng.choice(digit_choices)
        mid = Fraction(mm) * Fraction(2) ** me
        rad = Fraction(rm) * Fraction(2) ** re_
        text = tg.print_real(mid, rad, digits)
        recs.append({"mm": str(mm), "me": me, "rm": str(rm), "re": re_, "digits": digits, "text": text})
    write("print_real.jsonl", recs)


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
        return str(rng.randrange(1, 10 ** rng.randrange(20, 400)))
    return str(rng.choice([0, 1, 2, 3, 5, 6, 7, 12, 18, 36, 100, 216, 1000, rng.randrange(10 ** 6)]))


def r_int(rng, big=False):
    return rng.choice(["", "", "-"]) + zeros(rng) + r_uint(rng, big)


def r_urat(rng, big=False):
    s = zeros(rng) + r_uint(rng, big)
    if rng.random() < 0.5:
        s += "/" + zeros(rng) + r_uint(rng, big)
    return s


def r_rat(rng, big=False):
    return rng.choice(["", "", "-"]) + r_urat(rng, big)


def r_fin(rng):
    s = r_rat(rng, True)
    if rng.random() < 0.7:
        s += ws(rng) + (" " if rng.random() < 0.9 else "") + "mod" + ws(rng) + " " + r_urat(rng, True)
    return s


def r_udec_dyadic(rng):
    """A decimal literal whose value is dyadic with a small odd mantissa (so the exactness condition holds)."""
    m = rng.randrange(0, 1 << 20)
    k = rng.randrange(0, 12)
    val = Fraction(m, 2 ** k)
    D, E = dec_parts(val) if val != 0 else (0, 0)
    style = rng.randrange(3)
    if style == 0 or E >= 0:
        if E >= 0:
            s = str(D) + ("0" * E if E < 30 else "e%d" % E)
        else:
            s = str(D) + "e%d" % E
    else:
        ds = str(D)
        if len(ds) > -E:
            s = ds[:len(ds) + E] + "." + ds[len(ds) + E:]
        else:
            s = "0." + "0" * (-E - len(ds)) + ds
    return s


def r_udec_any(rng):
    ip = zeros(rng) + str(rng.randrange(0, 10 ** rng.randrange(1, 30)))
    s = ip
    if rng.random() < 0.5:
        s += "." + str(rng.randrange(0, 10 ** rng.randrange(1, 30))).rjust(rng.randrange(1, 5), "0")
    if rng.random() < 0.4:
        s += rng.choice("eE") + rng.choice(["", "+", "-"]) + str(rng.randrange(0, 60))
    return s


def r_real(rng):
    gen = r_udec_dyadic if rng.random() < 0.6 else r_udec_any
    s = rng.choice(["", "", "-"]) + gen(rng)
    if rng.random() < 0.6:
        s += ws(rng) + "+/-" + ws(rng) + (r_udec_dyadic(rng) if rng.random() < 0.6 else r_udec_any(rng))
    return s


def r_complex(rng):
    return ("(" + ws(rng) + r_real(rng) + ws(rng) + ")" + ws(rng) + "+" + ws(rng) + "(" + ws(rng) + r_real(rng)
            + ws(rng) + ")" + ws(rng) + "*" + ws(rng) + "i")


def r_text(rng, t):
    if t == "rat":
        return ws(rng) + r_rat(rng, True) + ws(rng)
    if t == "fball":
        if rng.random() < 0.3:
            return ws(rng) + r_rat(rng, True) + " mod " + r_urat(rng, True) + ws(rng)
        return ws(rng) + "(" + ws(rng) + "*" + ws(rng) + ";" + ws(rng) + r_fin(rng) + ws(rng) + ")" + ws(rng)
    if t == "adele":
        return ("(" + ws(rng) + r_real(rng) + ws(rng) + ";" + ws(rng) + r_fin(rng) + ws(rng) + ")")
    if t == "cadele":
        return ("(" + ws(rng) + r_complex(rng) + ws(rng) + ";" + ws(rng) + r_fin(rng) + ws(rng) + ")")
    raise ValueError(t)


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
    all_texts = []
    for t in ("rat", "fball", "adele", "cadele"):
        recs = []
        seen = set()
        n = 1500 if t in ("rat", "fball") else 1200
        while len(recs) < n:
            data = r_text(rng, t).encode("ascii")
            if rng.random() < 0.35:
                data = mutate(rng, data)
            if data in seen:
                continue
            seen.add(data)
            out = tg.canonical(t, data)
            rec = {"hex": data.hex(), "expected": out}
            if t in ("adele", "cadele") and not out.startswith("!"):
                rec["parts"] = parts_of(t, data)
            recs.append(rec)
            all_texts.append(data)
        write("text_%s.jsonl" % t, recs)
    return all_texts


# ------------------------------------------------------------------------------------------ classify

def gen_classify(rng, extra):
    inputs = []
    for name in tg.TYPES + ["dispatch", "dump"]:
        for _, data, _, _ in golden_rows(name):
            if len(data) <= 4096:
                inputs.append(data)
    base = list(inputs)
    for _ in range(3000):
        inputs.append(mutate(rng, rng.choice(base)))
    inputs += extra[::3]
    recs = []
    seen = set()
    for data in inputs:
        if data in seen:
            continue
        seen.add(data)
        recs.append({"hex": data.hex(), "expected": tg.classify(data)})
    write("classify.jsonl", recs)


def main():
    rng = random.Random(SEED)
    gen_real_parts()
    gen_print_real(rng)
    extra = gen_texts(rng)
    gen_classify(rng, extra)


if __name__ == "__main__":
    main()
