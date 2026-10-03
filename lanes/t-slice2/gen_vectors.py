#!/usr/bin/env python3
"""lanes/t-slice2/gen_vectors.py: the vectors of lane t-slice2 (the value form of adf_lball and adf_sball).

For every generated text of one of the two kinds it writes one JSON line with the verdict of the
reference proto/text_grammar.py (`canonical`), so that tests/test_text_local.c reads the status and the
canonical text of the reference and compares them with what the C reader and the C printer give.

Output: tests/ref/vectors/t-slice2/vectors_local.jsonl (one record per line):

    {"type": "lball"|"sball", "text": "<the bytes of the text>", "ref": "<the verdict of the reference>",
     "exact_real": false, "parts": [["<mid>", "<rad>"], ...]}

`parts` holds the exact balls of the real parts of the text, as the strings "p/q" of Python's Fraction:one pair for a real "inf" entry, two (the real and the imaginary part) for a complex one, and [] when the text has no archimedean entry. They are read from the reference (tg._real on its own tree), so the C test can require that the stored arb contains the exact interval of the text and that the printed interval contains it as well.

`ref` is the string `canonical` returns: `!OK` never happens; a status is `!NAME`, otherwise the canonical
text. `exact_real` is 1 when every real ball of the text is read by the C reader as an exact arb, that is
when its midpoint is dyadic with an odd mantissa of at most prec bits and its radius is dyadic with an odd
mantissa below 2^30 (docs/conventions.md 9.5, "Reading"; the tightness requirement). Only then can the C
printer be required to print the canonical text of the reference character for character: otherwise the C
value is a ball around the exact interval (gate finding G4, conventions 9.6) and the texts differ.

Usage:  python3 lanes/t-slice2/gen_vectors.py [count_per_kind]
(from the repository root; proto/ must be importable).
"""

import json
import os
import random
import sys
from fractions import Fraction

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import text_grammar as tg  # noqa: E402

PREC = 128
MAG_BITS = 30            # mag.h:117, the bound of a radius that is read exactly (conventions 9.5)

PRIMES = [2, 3, 5, 7, 65537, 2**64 - 59]


def frac_is_dyadic_small(q, bits):
    """1 if the rational q is dyadic with an odd mantissa of at most `bits` bits."""
    num, den = q.numerator, q.denominator
    while den % 2 == 0:
        den //= 2
    if den != 1:
        return 0
    m = abs(num)
    while m % 2 == 0 and m != 0:
        m //= 2
    return 1 if m.bit_length() <= bits else 0


def frac_is_dyadic_30(q):
    return frac_is_dyadic_small(q, MAG_BITS)


def rand_rat(rng, maxbits=6):
    num = rng.randint(-maxbits * 4, maxbits * 4)
    den = rng.choice([1, 1, 2, 3, 4, 5, 7, 8, 12, 25, 125])
    return Fraction(num, den)


def rand_dec_str(rng):
    """A decimal literal `dec` of conventions 9.1, sometimes with an exponent."""
    k = rng.randint(0, 3)
    ip = str(rng.randint(0, 999))
    fp = "".join(str(rng.randint(0, 9)) for _ in range(k))
    s = ip + ("." + fp if fp else "")
    if rng.random() < 0.3:
        s += rng.choice("eE") + rng.choice(["", "+", "-"]) + str(rng.randint(0, 6))
    if rng.random() < 0.3:
        s = "-" + s
    return s


def exact_real(rng, mode):
    """A real ball `dec ["+/-" udec]`, with mode 0 inexact (not read exactly by the C reader at PREC bits),
    1 exactly (a dyadic point of few digits), 2 exactly (a dyadic point with a dyadic radius below 2^30)."""
    if mode == 1:
        num = rng.randint(-40, 40)
        den = rng.choice([1, 2, 4, 8])
        m = Fraction(num, den)
        r = Fraction(0)
    elif mode == 2:
        e = rng.randint(-9, 9)
        m = Fraction(rng.randint(-50, 50), 2 ** rng.randint(0, 6)) * Fraction(2) ** e
        m = Fraction(m)
        if rng.random() < 0.5:
            k = rng.randint(0, 12)
            r = Fraction(2 * rng.randint(1, 12) + 1, 2**k)
        else:
            r = Fraction(0)
    else:
        # not read exactly: at least one of the two literals has a factor 3 (or another odd prime) in the
        # denominator of its exact value, so no arb at PREC bits is that ball
        m = rng.choice(["0.3", "1.7", "-2.5e-1", "12.25", "-0.7", "9e-1"])
        r = rng.choice(["0.3", "9e-1", "1e-9", "0.07", "1.5e-7", "0.00011", "3", "0"])
        return "%s +/- %s" % (m, r)
    a, b = fmt_exact(m), fmt_exact(r)
    if a is None:
        a = dec_of(m)                    # a decimal literal, inexact when m is not dyadic
    if b is None:
        b = dec_of(r)
    if a is None or b is None:
        return "0"
    return "%s +/- %s" % (a, b)


def fmt_exact(q):
    """The decimal of the rational q in positional notation (q is dyadic with a small exponent)."""
    num, den = q.numerator, q.denominator
    while den % 2 == 0:
        den //= 2
        num *= 2
    if den != 1:
        return None                      # not dyadic: the caller must not use it
    s = str(num)
    return s


def dec_of(q):
    """A decimal literal for the rational q, with an exponent when it is short."""
    num, den = q.numerator, q.denominator
    k = 0
    while den % 10 == 0 and den != 1:
        den //= 10
        k += 1
    while num % 10 == 0 and num != 0:
        num //= 10
        k += 1
    if den != 1:
        return None
    s = str(num)
    if k == 0:
        return s
    return "%se%d" % (s, -k)


def gen_lcoord(rng, p, want_exact):
    """A `lcoord` of conventions 9.2 for the prime p, and 1 if its real-ball-free form is what is asked."""
    style = rng.random()
    if style < 0.3:
        a = str(rng.randint(-20, 20))
    elif style < 0.5:
        a = "%d/%d" % (rng.randint(-40, 40), rng.choice([1, 2, 3, 4, 5, 7]))
    elif style < 0.6:
        a = rand_dec_str(rng)
    else:
        a = str(rng.randint(-9, 9))
    if rng.random() < 0.55:
        return "%s + O(%s^%s)" % (a, str(p), rng.choice(["", "", "-3", "-2", "-1", "0", "1", "2", "4"]))
    return a


def gen_lball(rng):
    p = rng.choice(PRIMES[:5] + [PRIMES[5]] if rng.random() < 0.1 else PRIMES[:5])
    body = gen_lcoord(rng, p, True)
    s = "[p=%s: %s]" % (str(p), body)
    if rng.random() < 0.25:               # whitespace
        s = s.replace(": ", " : ").replace(" + ", " + ").replace("[", "[ ")
    return s


def real_mode(rng):
    r = rng.random()
    return 0 if r < 0.5 else (1 if r < 0.65 else 2)


def gen_sentry(rng, p, want_exact):
    if rng.random() < 0.3:
        return "inf: %s" % exact_real(rng, real_mode(rng))
    if rng.random() < 0.08:
        return "inf: (%s) + (%s)*i" % (exact_real(rng, real_mode(rng)), exact_real(rng, 1))
    return "p=%s: %s" % (str(p), gen_lcoord(rng, p, True))


def gen_sball(rng):
    n = rng.randint(0, 4)
    ps = rng.sample(PRIMES[:5], min(n, 5))
    entries = []
    if rng.random() < 0.4:
        entries.append("inf: %s" % exact_real(rng, real_mode(rng)))
    for p in ps:
        entries.append("p=%s: %s" % (str(p), gen_lcoord(rng, p, True)))
    rng.shuffle(entries)
    s = "{" + "; ".join(entries) + "}"
    if rng.random() < 0.15:
        s = "{ " + s[1:-1] + " }"
    return s


MUTATIONS = ["drop", "swap", "upper", "insert", "cut", "nul", "digit", "bracket"]


def mutate(rng, s):
    if not s:
        return s
    kind = rng.choice(MUTATIONS)
    i = rng.randrange(len(s))
    if kind == "drop":
        return s[:i] + s[i + 1:]
    if kind == "cut":
        return s[:i]
    if kind == "swap" and i + 1 < len(s):
        return s[:i] + s[i + 1] + s[i] + s[i + 2:]
    if kind == "upper":
        return s[:i] + s[i].upper() + s[i + 1:]
    if kind == "insert":
        return s[:i] + rng.choice(";{}[]()+-,^*/: pOimf5") + s[i:]
    if kind == "nul":
        return s[:i] + "\x00" + s[i + 1:]
    if kind == "digit":
        return s[:i] + rng.choice("0123456789") + s[i + 1:]
    return s[:i] + rng.choice("{}[]()") + s[i:]


def exact_real_of(data, kind):
    """1 if the C reader reads every real ball of the text exactly at PREC bits (conventions 9.5).

    The exact intervals come from the reference itself (tg._real, tg._complex on its own tree), not from a
    second reading of the text here."""
    if kind == "lball":
        return 1                        # no real part at all
    try:
        s = tg._prep(data, tg.DEFAULT_LIMITS)
        tree = tg._syntax(kind, s)
    except tg.TextError:
        return 0
    for e in tree[1][1]:
        if e[0] == "R":
            parts = [tg._real(e[1])]
        elif e[0] == "C":
            parts = [tg._real(e[1][1]), tg._real(e[1][2])]
        else:
            continue
        for (mid, rad) in parts:
            if not frac_is_dyadic_small(mid, PREC) or not frac_is_dyadic_30(rad):
                return 0
    return 1


def exact_parts(data, kind):
    """The exact balls (mid, rad) of every real part of the text, from the reference itself, as the
    strings "p/q" (conventions 9.5: the text denotes the exact interval [mid - rad, mid + rad]). One pair
    for a real entry, two (real, imaginary) for a complex one; [] when there is no archimedean entry and
    for a local ball."""
    if kind == "lball":
        return []
    try:
        s = tg._prep(data, tg.DEFAULT_LIMITS)
        tree = tg._syntax(kind, s)
    except tg.TextError:
        return []
    out = []
    for e in tree[1][1]:
        if e[0] == "R":
            out.append(tg._real(e[1]))
        elif e[0] == "C":
            out.append(tg._real(e[1][1]))
            out.append(tg._real(e[1][2]))
    return [[str(m), str(r)] for (m, r) in out]


def verdict(kind, text):
    return tg.canonical(kind, text)


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
    rng = random.Random(20260930)
    out = []
    kinds = {"lball": gen_lball, "sball": gen_sball}
    for kind in ("lball", "sball"):
        seen = set()
        while len([o for o in out if o["type"] == kind]) < n:
            s = kinds[kind](rng)
            if rng.random() < 0.45:
                for _ in range(rng.randint(1, 3)):
                    s = mutate(rng, s)
            if s in seen:
                continue
            seen.add(s)
            b = s.encode("utf-8", "surrogateescape")
            ref = verdict(kind, b)
            ok = ref.startswith("!")
            out.append({
                "type": kind,
                "text": b.decode("utf-8", "surrogateescape"),
                "ref": ref,
                "exact_real": bool(ok or exact_real_of(b, kind)),
                "parts": exact_parts(b, kind) if not ok else [],
            })
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tests", "ref", "vectors", "t-slice2",
                        "vectors_local.jsonl")
    with open(path, "w") as f:
        for o in out:
            f.write(json.dumps(o, ensure_ascii=False) + "\n")
    bad = sum(1 for o in out if o["ref"].startswith("!"))
    print("wrote %d records (%d of them a status) to %s" % (len(out), bad, path))
    for kind in ("lball", "sball"):
        ks = [o for o in out if o["type"] == kind]
        print("  %s: %d records, %d statuses, %d not exact" %
              (kind, len(ks), sum(1 for o in ks if o["ref"].startswith("!")),
               sum(1 for o in ks if o["exact_real"] == 0)))


if __name__ == "__main__":
    main()