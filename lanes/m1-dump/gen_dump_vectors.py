#!/usr/bin/env python3
"""Vectors for the dump form (lane m1-dump): tests/ref/vectors/m1-dump/dump_ref.jsonl.

The oracle is the reference proto/text_grammar.py (dump_roundtrip, dump_contexts; docs/conventions.md 10).
Each record is
    {"text": T, "max_items": n, "max_len": n, "roundtrip": R, "contexts": C}
with R the re-dumped text or "!STATUS" (proto/text_grammar.py:1472-1479) and C the list of context
occurrences [[K, [q_1, ..., q_k]], ...] or "!STATUS" (proto/text_grammar.py:1482-1490).

The texts are valid dumps of the bodies rat, fball (g and l), scaled (x and s), adele, cadele, modctx and
qclass (lift and pieces), built from random data, and mutations of them (a token replaced, dropped, doubled,
the separators or the header changed). Only printable ASCII is written, so the JSON strings need no escape.
Deterministic: seed 20260928. Run from the repository root:

    python3 lanes/m1-dump/gen_dump_vectors.py
"""
import json
import os
import random
import sys
from math import gcd

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "proto"))
import text_grammar as tg  # noqa: E402

R = random.Random(20260928)
WORD = 2 ** 64
OUT = os.path.join("tests", "ref", "vectors", "m1-dump", "dump_ref.jsonl")


def hx(n):
    return ("-" if n < 0 else "") + format(abs(n), "x")


def rbits():
    """A bit size: mostly small, sometimes a word, sometimes 4096 bits."""
    c = R.random()
    if c < 0.5:
        return R.randint(1, 20)
    if c < 0.8:
        return R.randint(21, 70)
    if c < 0.95:
        return R.randint(71, 400)
    return 4096


def rint(signed=True):
    v = R.getrandbits(rbits())
    if signed and R.random() < 0.5:
        v = -v
    return v


def rpos():
    v = 0
    while v == 0:
        v = R.getrandbits(rbits())
    return v


def rat_canon(num, den):
    g = gcd(num, den)
    return num // g, den // g


def blocks(k):
    """k pairwise coprime words >= 2, in random order."""
    out = []
    tries = 0
    while len(out) < k and tries < 10000:
        tries += 1
        c = R.random()
        if c < 0.4:
            q = R.choice([2, 3, 4, 5, 7, 8, 9, 11, 13, 16, 25, 27, 49, 121])
        elif c < 0.8:
            q = R.randint(2, 2 ** 20)
        else:
            q = R.randint(2 ** 60, WORD - 1)
        if all(gcd(q, p) == 1 for p in out):
            out.append(q)
    return out


def ctx_tokens(K, q):
    return [hx(K), hx(len(q))] + [hx(x) for x in q]


def rctx(need_blocks):
    k = R.choice([1, 1, 2, 3, 5]) if need_blocks or R.random() < 0.7 else 0
    if k == 0:
        K = R.choice([1, 6, 7, 2 ** 64, 2 ** 70 + 1, rpos()])
        return K, []
    q = blocks(k)
    K = 1
    for x in q:
        K *= x
    return K, q


def fb_global():
    c = R.random()
    d = rpos()
    if c < 0.3:
        A = rint()
        g = gcd(A, d)
        return ["g", hx(A // g), "0", hx(d // g)]
    H = rpos()
    A = R.randrange(H)
    g = gcd(gcd(A, H), d)
    return ["g", hx(A // g), hx(H // g), hx(d // g)]


def fb_local(K=None, q=None, d=None):
    if K is None:
        K, q = rctx(True)
    if d is None:
        d = rpos()
    res = [R.randrange(x) for x in q]
    return ["l", hx(d)] + ctx_tokens(K, q) + [hx(r) for r in res]


def fb():
    return fb_global() if R.random() < 0.5 else fb_local()


def rarb():
    """An arb in the canonical dump form (conventions 10.2): odd mantissas or 0 0; radius mantissa < 2^30."""
    if R.random() < 0.15:
        mid = ["0", "0"]
    else:
        m = R.getrandbits(rbits()) | 1
        if R.random() < 0.5:
            m = -m
        e = R.choice([0, 1, -1, R.randint(-200, 200), R.randint(-2 ** 40, 2 ** 40), R.randint(-2 ** 70, 2 ** 70)])
        mid = [hx(m), hx(e)]
    if R.random() < 0.3:
        rad = ["0", "0"]
    else:
        m = R.choice([1, 3, 2 ** 29 + 1, 2 ** 30 - 1, R.getrandbits(30) | 1])
        e = R.choice([0, -1, R.randint(-200, 200), R.randint(-2 ** 40, 2 ** 40)])
        rad = [hx(m), hx(e)]
    return mid + rad


def valid_text():
    c = R.randrange(9)
    if c == 0:
        num = rint()
        den = rpos()
        num, den = rat_canon(num, den)
        body = ["rat", hx(num), hx(den)]
    elif c == 1:
        body = ["fball"] + fb()
    elif c == 2:
        K, q = rctx(False)
        if R.random() < 0.5:
            num, den = rat_canon(rint(), rpos())
            body = ["scaled", "x", hx(num), hx(den)] + ctx_tokens(K, q)
        else:
            num, den = rat_canon(rpos(), rpos())
            body = ["scaled", "s", hx(num), hx(den), hx(R.randrange(K))] + ctx_tokens(K, q)
    elif c == 3:
        body = ["adele", "1"] + rarb() + fb()
    elif c == 4:
        body = ["cadele", "1"] + rarb() + rarb() + fb()
    elif c == 5:
        K, q = rctx(False)
        body = ["modctx"] + ctx_tokens(K, q)
    elif c == 6:
        body = ["qclass", "lift", "1"] + rarb() + fb()
    else:
        n = R.randint(1, 4)
        pieces = []
        for _ in range(n):
            # midpoint j/16 in [0, 1], small radius; finite part with d = 1
            j = R.randint(0, 16)
            if j == 0:
                mid = ["0", "0"]
            else:
                m, e = j, -4
                while m % 2 == 0:
                    m //= 2
                    e += 1
                mid = [hx(m), hx(e)]
            rad = R.choice([["0", "0"], ["1", "-6"], ["3", "-8"]])
            if R.random() < 0.5:
                H = R.randint(1, 50)
                f = ["g", hx(R.randrange(H)), hx(H), "1"]
            else:
                K, q = rctx(True)
                f = fb_local(K, q, 1)
            pieces.append(["1"] + mid + rad + f)
        # canonical order and no two equal keys (proto/text_grammar.py:1358-1368)
        keyed = {}
        for p in pieces:
            node = tg._dump_syntax("adf1 Q qclass pieces 1 " + " ".join(p), tg.DEFAULT_LIMITS)
            arbs, f = node[2][0]
            mid_v, rad_v = tg._arb_value(arbs[0])
            A, H, _ = tg._v_fb(f)
            keyed[(mid_v - rad_v, mid_v + rad_v, H, A)] = p
        pieces = [keyed[k] for k in sorted(keyed)]
        body = ["qclass", "pieces", hx(len(pieces))] + [t for p in pieces for t in p]
    return "adf1 Q " + " ".join(body)


def mutate(text):
    toks = text.split(" ")
    c = R.randrange(14)
    i = R.randrange(3, len(toks)) if len(toks) > 3 else len(toks) - 1
    if c == 0:
        toks[i] = hx(rint())
    elif c == 1:
        toks[i] = R.choice(["0", "-0", "1", "-1", "2", "g", "A", "ffffffffffffffff", "10000000000000000"])
    elif c == 2:
        toks[i] = toks[i].upper()
    elif c == 3:
        toks[i] = "0" + toks[i]
    elif c == 4:
        del toks[i]
    elif c == 5:
        toks.insert(i, toks[i])
    elif c == 6:
        return text + " "
    elif c == 7:
        j = R.randrange(len(text))
        return text[:j] + " " + text[j:]
    elif c == 8:
        toks[0] = R.choice(["adf2", "adf01", "adf", "ADF1", "adf10"])
    elif c == 9:
        toks[1] = R.choice(["K", "q", "QQ", "F3(T)"])
    elif c == 10:
        toks[2] = R.choice(["rat", "fball", "scaled", "adele", "cadele", "modctx", "qclass", "ucoset"])
    elif c == 11:
        toks[i] = hx(int(toks[i], 16) + R.choice([-1, 1])) if tg.RE_HEX.match(toks[i]) else toks[i]
    elif c == 12:
        toks[i] = "-" + toks[i] if not toks[i].startswith("-") else toks[i][1:]
    else:
        toks.append(R.choice(["0", "1", "g"]))
    return " ".join(toks)


def record(text, max_items=1048576, max_len=1048576):
    lim = tg.Limits(max_len=max_len, max_items=max_items)
    rt = tg.dump_roundtrip(text, lim)
    cx = tg.dump_contexts(text, lim)
    if not isinstance(cx, str):
        cx = [[K, list(q)] for K, q in cx]
    return {"text": text, "max_items": max_items, "max_len": max_len, "roundtrip": rt, "contexts": cx}


def main():
    recs = []
    for _ in range(700):
        t = valid_text()
        recs.append(record(t))
        assert recs[-1]["roundtrip"] == t, (t, recs[-1]["roundtrip"])
        for _ in range(2):
            recs.append(record(mutate(t)))
    # limits: a block count over max_items, a text over max_len
    for _ in range(40):
        t = valid_text()
        recs.append(record(t, max_items=R.choice([0, 1, 2])))
        recs.append(record(t, max_len=R.choice([len(t) - 1, len(t)])))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        for r in recs:
            f.write(json.dumps(r, sort_keys=True) + "\n")
    n_ok = sum(1 for r in recs if not r["roundtrip"].startswith("!"))
    print("%d records, %d valid, written to %s" % (len(recs), n_ok, OUT))


if __name__ == "__main__":
    main()
