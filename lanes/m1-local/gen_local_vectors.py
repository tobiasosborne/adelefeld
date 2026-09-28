#!/usr/bin/env python3
"""Generate tests/ref/vectors/m1-local/local.jsonl from tests/ref/adfref/local_ref.py.

Deterministic (seed 20260928). Run from the repository root:

    python3 lanes/m1-local/gen_local_vectors.py

Record format (one JSON object per line):
  "op"        one of to_global, set_local, set_local_enclose, neg, add, sub, mul, scale, div_rat,
              equal_set, overlaps, contains, compare, contains_rat
  "blocks"    the blocks of the context, in order (every record has one context)
  "a", "b"    values: {"backend": "local", "d": d, "res": [...]} or
              {"backend": "global", "A": A, "H": H, "d": d} (canonical)
  "same_ctx"  binary operations with two local inputs: true if they share one context pointer,
              false if b is in a second context with the same blocks (conventions 4.6)
  "q"         {"num": m, "den": n} for scale, div_rat, contains_rat
  "result"    a value as above (its backend is part of the expectation), or a boolean, or the
              comparison code (0, 1, 2) for compare
  "lost"      set_local_enclose only
  "status"    instead of "result" when the operation returns a status (DOMAIN, UNSUPPORTED,
              NOT_UNIT)
  "canonical" for every value result: the canonical global triple {"A", "H", "d"} of its set
"""
import json
import os
import random
import sys
from fractions import Fraction

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tests", "ref"))
from adfref import fball as F  # noqa: E402
from adfref import local_ref as L  # noqa: E402
from adfref.modctx_ref import is_prime  # noqa: E402

OUT = "tests/ref/vectors/m1-local/local.jsonl"
rng = random.Random(20260928)


def primes_from(start, count):
    out, p = [], start
    while len(out) < count:
        if is_prime(p):
            out.append(p)
        p += 1
    return out


SMALL_PRIMES = primes_from(2, 128)

CONTEXTS = {
    "c2": (2,),
    "c4": (4,),
    "c6": (6,),
    "c9": (9,),
    "c12": (12,),
    "c16": (16,),
    "c4_9": (4, 9),
    "c9_4": (9, 4),
    "c8_3_25": (8, 3, 25),
    "cbig1": (2**64 - 59,),
    "cpow2_63": (2**63,),
    "cbig2": (2**61 - 1, 3**40),
    "c64": tuple(p ** (1 + (i % 3)) for i, p in enumerate(SMALL_PRIMES[:64])),
    "c128": tuple(p ** (1 + (i % 2)) if p < 1000 else p for i, p in enumerate(SMALL_PRIMES[:128])),
    "c64big": tuple(primes_from(2**62, 64)),
}
BLOCK_COUNTS = {name: len(b) for name, b in CONTEXTS.items()}


def enc_value(v):
    if isinstance(v, L.Local):
        return {"backend": "local", "d": v.d, "res": list(v.res)}
    return {"backend": "global", "A": v.A, "H": v.H, "d": v.d}


def enc_canon(v):
    g = L.to_global(v)
    return {"A": g.A, "H": g.H, "d": g.d}


def enc_q(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def rand_d(big=False):
    if big:
        return rng.getrandbits(4096) | 1 if rng.random() < 0.5 else rng.getrandbits(3000) * 6 + 2
    choice = rng.random()
    if choice < 0.3:
        return rng.choice([1, 2, 3, 4, 6, 8, 9, 12, 16, 25, 27, 36])
    if choice < 0.6:
        d = 1
        for _ in range(rng.randint(1, 4)):
            d *= rng.choice(SMALL_PRIMES[:12])
        return d
    return rng.randint(1, 10**6)


def rand_res(ctx):
    res = []
    for q in ctx.blocks:
        u = rng.random()
        if u < 0.15:
            res.append(0)
        elif u < 0.4 and q < 10**7:
            divs = [t for t in range(2, min(q, 5000) + 1) if q % t == 0]
            t = rng.choice(divs) if divs else 1
            res.append((t * rng.randrange(q)) % q)
        else:
            res.append(rng.randrange(q))
    return res


def rand_local(ctx, big=False):
    return L.Local(ctx, rand_d(big), rand_res(ctx))


def rand_global(big=False):
    u = rng.random()
    if u < 0.1:
        n, d = rng.randint(-50, 50), rng.randint(1, 20)
        return F.Fball(n, 0, d)
    bits = 4096 if big else 40
    H = rng.getrandbits(bits) + 1
    d = rng.getrandbits(bits // 2) + 1
    A = rng.getrandbits(bits) - (1 << (bits - 1))
    return F.Fball(A, H, d)


records = []


def emit(rec, fn):
    try:
        out = fn()
    except L.LocalError as e:
        rec["status"] = e.status
        records.append(rec)
        return
    if isinstance(out, tuple):
        val, lost = out
        rec["result"] = enc_value(val)
        rec["canonical"] = enc_canon(val)
        rec["lost"] = bool(lost)
    elif isinstance(out, (L.Local, F.Fball)):
        rec["result"] = enc_value(out)
        rec["canonical"] = enc_canon(out)
    elif isinstance(out, bool):
        rec["result"] = out
    else:
        rec["result"] = int(out)
    records.append(rec)


def hand_written():
    c4 = L.Ctx((4,))
    c2 = L.Ctx((2,))
    c6 = L.Ctx((6,))
    # Proposition 25, brief: A = d = 2 with block 4; the set is 1 + 2 Zhat
    x = L.Local(c4, 2, (2,))
    emit({"op": "to_global", "blocks": [4], "a": enc_value(x)}, lambda: L.to_global(x))
    emit({"op": "set_local", "blocks": [4], "a": enc_value(F.Fball(1, 2, 1))},
         lambda: L.set_local(F.Fball(1, 2, 1), c4))
    emit({"op": "set_local", "blocks": [4], "a": enc_value(F.Fball(3, 2, 1))},
         lambda: L.set_local(F.Fball(3, 2, 1), c4))
    # Proposition 24: (2; 2) in (4), canonical (1, 2, 1)
    emit({"op": "equal_set", "blocks": [4], "a": enc_value(x), "b": enc_value(F.Fball(1, 2, 1))},
         lambda: L.equal_set(x, F.Fball(1, 2, 1)))
    # Summary 26: the canonical H changes after a sum
    y = L.Local(c2, 2, (1,))
    emit({"op": "add", "blocks": [2], "a": enc_value(y), "b": enc_value(y), "same_ctx": True},
         lambda: L.add(y, y))
    emit({"op": "mul", "blocks": [6], "a": enc_value(L.Local(c6, 2, (1,))),
          "b": enc_value(L.Local(c6, 3, (2,))), "same_ctx": True},
         lambda: L.mul(L.Local(c6, 2, (1,)), L.Local(c6, 3, (2,))))
    # Proposition 22 example: h = 2 does not divide d e = 1: global 4 + 8 Zhat
    z = L.Local(c4, 1, (2,))
    emit({"op": "mul", "blocks": [4], "a": enc_value(z), "b": enc_value(z), "same_ctx": True},
         lambda: L.mul(z, z))
    # h = 2 divides d e = 4: local
    w = L.Local(c4, 2, (2,))
    emit({"op": "mul", "blocks": [4], "a": enc_value(w), "b": enc_value(w), "same_ctx": True},
         lambda: L.mul(w, w))
    # exact inputs to the conversions
    for blocks in ((4,), (4, 9)):
        ctx = L.Ctx(blocks)
        for b in (F.Fball(0, 0, 1), F.Fball(-7, 0, 3)):
            emit({"op": "set_local", "blocks": list(blocks), "a": enc_value(b)}, lambda: L.set_local(b, ctx))
            emit({"op": "set_local_enclose", "blocks": list(blocks), "a": enc_value(b)},
                 lambda: L.set_local_enclose(b, ctx))
    # Proposition 20: 1/2 + (1/2) Zhat in (4): K/R = 8, e = 2, d0 = 8, not lost
    b = F.Fball.from_center_radius(Fraction(1, 2), Fraction(1, 2))
    emit({"op": "set_local_enclose", "blocks": [4], "a": enc_value(b)}, lambda: L.set_local_enclose(b, c4))
    # lost: 1 + 3 Zhat into (4): K/R = 4/3, n = 4, d0 = 4, the ball 1 + Zhat
    b = F.Fball(1, 3, 1)
    emit({"op": "set_local_enclose", "blocks": [4], "a": enc_value(b)}, lambda: L.set_local_enclose(b, c4))
    emit({"op": "set_local", "blocks": [4], "a": enc_value(b)}, lambda: L.set_local(b, c4))
    # scalars and div_rat by 0
    emit({"op": "scale", "blocks": [4], "a": enc_value(w), "q": enc_q(0)}, lambda: L.scale(w, 0))
    emit({"op": "div_rat", "blocks": [4], "a": enc_value(w), "q": enc_q(0)}, lambda: L.div_rat(w, 0))
    emit({"op": "scale", "blocks": [4], "a": enc_value(w), "q": enc_q(Fraction(-2, 3))},
         lambda: L.scale(w, Fraction(-2, 3)))
    emit({"op": "scale", "blocks": [4], "a": enc_value(w), "q": enc_q(Fraction(3, 1))},
         lambda: L.scale(w, 3))
    emit({"op": "div_rat", "blocks": [4], "a": enc_value(w), "q": enc_q(Fraction(3, 2))},
         lambda: L.div_rat(w, Fraction(3, 2)))


def random_records(name, n, big=False):
    blocks = CONTEXTS[name]
    ctx = L.Ctx(blocks)
    other = L.Ctx(blocks)
    bl = list(blocks)
    for _ in range(n):
        x = rand_local(ctx, big and rng.random() < 0.3)
        y = rand_local(ctx, big and rng.random() < 0.3)
        g = rand_global(big)
        op = rng.choice(["to_global", "set_local", "set_local_r", "enclose", "neg", "add", "sub",
                         "mul", "mul", "mul", "scale", "div_rat", "pred", "mixed", "other"])
        if op == "to_global":
            emit({"op": "to_global", "blocks": bl, "a": enc_value(x)}, lambda: L.to_global(x))
        elif op == "set_local":
            gx = L.to_global(x)
            emit({"op": "set_local", "blocks": bl, "a": enc_value(gx)}, lambda: L.set_local(gx, ctx))
        elif op == "set_local_r":
            emit({"op": "set_local", "blocks": bl, "a": enc_value(g)}, lambda: L.set_local(g, ctx))
        elif op == "enclose":
            src = g if rng.random() < 0.5 else L.to_global(x)
            emit({"op": "set_local_enclose", "blocks": bl, "a": enc_value(src)},
                 lambda: L.set_local_enclose(src, ctx))
        elif op == "neg":
            emit({"op": "neg", "blocks": bl, "a": enc_value(x)}, lambda: L.neg(x))
        elif op in ("add", "sub", "mul"):
            fn = {"add": L.add, "sub": L.sub, "mul": L.mul}[op]
            if op == "mul" and rng.random() < 0.5:
                # force shared factors: y's residues are multiples of gcd-ish parts of x's
                y = L.Local(ctx, rand_d(), [(r * rng.randint(0, 3)) % q for q, r in zip(blocks, x.res)])
            emit({"op": op, "blocks": bl, "a": enc_value(x), "b": enc_value(y), "same_ctx": True},
                 lambda: fn(x, y))
        elif op in ("scale", "div_rat"):
            m = rng.choice([0, 1, -1, 2, -2, 3, 6, -12, rng.randint(-10**6, 10**6)])
            nn = rng.choice([1, 1, 2, 5, 7, rng.randint(1, 10**6)])
            q = Fraction(m, nn)
            fn = L.scale if op == "scale" else L.div_rat
            emit({"op": op, "blocks": bl, "a": enc_value(x), "q": enc_q(q)}, lambda: fn(x, q))
        elif op == "pred":
            b2 = rng.choice([y, L.to_global(y), L.to_global(x), x, g,
                             L.Local(ctx, x.d, x.res), L.add(x, L.Local(ctx, 1, [0] * len(blocks)))])
            which = rng.choice(["equal_set", "overlaps", "contains", "compare", "contains_rat"])
            if which == "contains_rat":
                gx = L.to_global(x)
                r = gx.center + gx.radius * rng.randint(-3, 3) if rng.random() < 0.6 else \
                    Fraction(rng.randint(-100, 100), rng.randint(1, 30))
                emit({"op": "contains_rat", "blocks": bl, "a": enc_value(x), "q": enc_q(r)},
                     lambda: L.contains_rat(x, r))
            else:
                fn = getattr(L, which)
                first, second = (x, b2) if rng.random() < 0.5 else (b2, x)
                emit({"op": which, "blocks": bl, "a": enc_value(first), "b": enc_value(second),
                      "same_ctx": True}, lambda: fn(first, second))
        elif op == "mixed":
            which = rng.choice(["add", "sub", "mul"])
            fn = {"add": L.add, "sub": L.sub, "mul": L.mul}[which]
            first, second = (x, g) if rng.random() < 0.5 else (g, x)
            emit({"op": which, "blocks": bl, "a": enc_value(first), "b": enc_value(second),
                  "same_ctx": True}, lambda: fn(first, second))
        else:
            which = rng.choice(["add", "sub", "mul", "equal_set"])
            fn = {"add": L.add, "sub": L.sub, "mul": L.mul, "equal_set": L.equal_set}[which]
            yo = L.Local(other, y.d, y.res)
            emit({"op": which, "blocks": bl, "a": enc_value(x), "b": enc_value(yo), "same_ctx": False},
                 lambda: fn(x, yo))


def main():
    hand_written()
    for name in ("c2", "c4", "c6", "c9", "c12", "c16", "c4_9", "c9_4", "c8_3_25"):
        random_records(name, 150)
    for name in ("cbig1", "cpow2_63", "cbig2"):
        random_records(name, 100)
        random_records(name, 15, big=True)
    for name in ("c64", "c128", "c64big"):
        random_records(name, 60)
        random_records(name, 8, big=True)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        for rec in records:
            f.write(json.dumps(rec, separators=(", ", ": ")) + "\n")
    counts = {}
    for rec in records:
        counts[rec["op"]] = counts.get(rec["op"], 0) + 1
    print("%d records: %s" % (len(records), ", ".join("%s %d" % kv for kv in sorted(counts.items()))))
    loc = sum(1 for r in records if isinstance(r.get("result"), dict) and r["result"]["backend"] == "local")
    print("results local %d, statuses %d" % (loc, sum(1 for r in records if "status" in r)))


if __name__ == "__main__":
    main()
