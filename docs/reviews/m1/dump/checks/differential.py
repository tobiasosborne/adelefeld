"""Deterministic grammar generator, mutations, reference differential and independent predicates."""
import collections
import ctypes as C
from fractions import Fraction
import importlib.util
import math
from pathlib import Path
import random
import sys

ROOT = Path(__file__).resolve().parents[5]
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("grammar", ROOT / "proto/text_grammar.py")
ref = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = ref
spec.loader.exec_module(ref)

class Limits(C.Structure):
    _fields_ = [("max_len", C.c_size_t), ("max_exp10", C.c_long),
                ("max_prec", C.c_long), ("max_items", C.c_long)]

lib = C.CDLL(str(HERE / "bridge.so"))
lib.review_probe.argtypes = [C.c_char_p, C.c_size_t, C.c_int, C.c_size_t,
                            C.POINTER(Limits), C.POINTER(C.c_int)]
lib.review_probe.restype = C.c_int
NAMES = {0: "OK", 7: "DOMAIN", 8: "UNSUPPORTED", 9: "PARSE", 10: "LIMIT"}
KINDS = "rat fball scaled adele cadele ucoset idele idclass lball sball qclass ffun rfun char modctx".split()
rng = random.Random(2026092807)

def hx(n):
    return format(n, "x")

def ctx():
    qs = rng.choice([[], [2], [6], [2, 3], [3, 2], [4, 9, 25], [5, 7, 11]])
    k = math.prod(qs) if qs else rng.choice([1, 2, 6, 2**70+3])
    return [hx(k), hx(len(qs))] + list(map(hx, qs)), qs, k

def arb():
    m = rng.choice([0, 1, -1, 3, -17, 2**90+1])
    r = rng.choice([0, 1, 3, 2**29+1, 2**30-1])
    return [hx(m), hx(rng.randint(-50, 50) if m else 0),
            hx(r), hx(rng.randint(-50, 50) if r else 0)]

def acb():
    return arb() + arb()

def fb(local=None):
    if local is None:
        local = rng.randrange(2)
    if local:
        c, qs, k = ctx()
        if qs:
            return ["l", hx(rng.randint(1, 16))] + c + [hx(rng.randrange(q)) for q in qs]
    a, h, d = rng.randint(-100, 100), rng.randint(0, 20), rng.randint(1, 20)
    if h:
        a %= h
    g = math.gcd(math.gcd(a, h), d)
    return ["g", hx(a//g), hx(h//g), hx(d//g)]

def lb():
    p = rng.choice([2, 3, 5, 7])
    return [hx(p), "x", "1", "1", hx(rng.randint(-8, 8))]

def body(kind):
    q = Fraction(rng.randint(-99, 99), rng.randint(1, 30))
    rat = [hx(q.numerator), hx(q.denominator)]
    if kind == "rat": return rat
    if kind == "fball": return fb()
    if kind == "scaled":
        c, qs, k = ctx()
        return (["x"] + rat if rng.randrange(2) else ["s", "1", "3", hx(rng.randrange(k))]) + c
    if kind == "adele": return ["1"] + arb() + fb()
    if kind == "cadele": return ["1"] + acb() + fb()
    if kind == "ucoset": return ["1", hx(rng.randint(0, 40))]
    if kind == "idele": return ["1"] + arb() + ["1", "3", "1", "6"]
    if kind == "idclass": return arb() + ["1", "6"]
    if kind == "lball": return lb()
    if kind == "sball":
        n = rng.randrange(4)
        a = rng.choice(["n", "r", "c"])
        return [a] + ({"n": [], "r": arb(), "c": acb()}[a]) + [hx(n)] + sum([lb() for _ in range(n)], [])
    if kind == "qclass":
        if rng.randrange(2): return ["lift", "1"] + arb() + fb()
        n = rng.randrange(4)
        return ["pieces", hx(n)] + sum([["1"] + arb() + fb() for _ in range(n)], [])
    if kind == "ffun":
        d, m = rng.randrange(3), rng.randrange(3)
        return [hx(d), hx(m)] + sum([acb() for _ in range(d*m)], [])
    if kind == "rfun":
        n = rng.randrange(3)
        return [hx(n)] + sum([["1"] + acb() + ["1", "0", "0", "0", "0", "0", "0", "0"]
                             + acb() + acb() for _ in range(n)], [])
    if kind == "char": return [hx(rng.choice([1, 3, 4, 5, 8, 12, 101])), "1"] + acb()
    if kind == "modctx": return ctx()[0]
    raise AssertionError(kind)

# This parser is independent of the project reference. It evaluates the stated predicates directly.
def own_valid(text):
    t = iter(text.decode().split()[2:])
    kind = next(t)
    def integer(): return int(next(t), 16)
    def rational():
        a, d = integer(), integer()
        return d > 0 and math.gcd(a, d) == 1, a, d
    def context():
        k, n = integer(), integer()
        qs = [integer() for _ in range(n)]
        ok = k >= 1 and all(2 <= q < 2**64 for q in qs)
        ok &= all(math.gcd(a, b) == 1 for i, a in enumerate(qs) for b in qs[:i])
        return ok and (not qs or math.prod(qs) == k), k, qs
    def ball():
        form = next(t)
        if form == "g":
            a, h, d = integer(), integer(), integer()
            return d > 0 and h >= 0 and (not h or 0 <= a < h) and math.gcd(math.gcd(a, h), d) == 1
        d = integer()
        ok, k, qs = context()
        rs = [integer() for _ in qs]
        return ok and bool(qs) and d > 0 and all(0 <= r < q for r, q in zip(rs, qs))
    def real():
        m, e, r, re = [integer() for _ in range(4)]
        return (m % 2 != 0 or (m == e == 0)) and (r == re == 0 or 0 < r < 2**30 and r % 2 == 1)
    if kind == "rat": ok = rational()[0]
    elif kind == "fball": ok = ball()
    elif kind == "scaled":
        form = next(t)
        ok, a, d = rational()
        u = integer() if form == "s" else 0
        c, k, qs = context()
        ok &= c and (form == "x" or a > 0 and 0 <= u < k)
    elif kind in ("adele", "cadele"):
        n = integer()
        arbs = [real() for _ in range(n * (2 if kind == "cadele" else 1))]
        b = ball()
        ok = n == 1 and all(arbs) and b
    else: raise AssertionError(kind)
    assert next(t, None) is None
    return ok

counts = collections.Counter()
mismatches = []
contract = []
own_count = 0
for i in range(150000):
    kind = KINDS[i % 15]
    toks = ["adf1", "Q", kind] + body(kind)
    mutation = rng.randrange(10)
    if mutation < 3:
        pos = rng.randrange(len(toks))
        toks[pos] = rng.choice(["-1", "0", "00", "-0", "2", "10000000000000000", "z", "40000001"])
    elif mutation == 3:
        del toks[rng.randrange(len(toks))]
    elif mutation == 4:
        toks.insert(rng.randrange(len(toks)+1), rng.choice(["0", "1", " "]))
    text = " ".join(toks).encode()
    if mutation == 5:
        p = rng.randrange(len(text)+1)
        text = text[:p] + rng.choice([b"\0", b"\t", b"\xff", b"\n", b" "]) + text[p:]
    lim = Limits(1048576, 100000, 100000, 1048576)
    if i % 13 == 0: lim.max_items = rng.randrange(4)
    if i % 31 == 0: lim.max_prec = rng.randrange(4)
    if i % 47 == 0: lim.max_len = len(text)-1
    rlim = ref.Limits(max_len=lim.max_len, max_prec=lim.max_prec, max_items=lim.max_items)
    expected = ref.modctx_new_from_dump(text, 0, rlim)
    expected = expected[1:] if isinstance(expected, str) else "OK"
    trouble = C.c_int()
    actual = NAMES[lib.review_probe(text, len(text), KINDS.index(kind) if kind in KINDS[:5] else -1,
                                  0, C.byref(lim), C.byref(trouble))]
    counts[(kind, actual)] += 1
    if expected != actual: mismatches.append((i, text.decode(errors="backslashreplace"), expected, actual))
    if trouble.value: contract.append((i, trouble.value, text))
    if kind in KINDS[:5] and mutation >= 6:
        assert own_valid(text)
        own_count += 1

print("texts=150000 bodies=15 seed=2026092807")
print(f"reference_mismatches={len(mismatches)} contract_failures={len(contract)} independent_predicates={own_count}")
for kind in KINDS:
    print(kind, " ".join(f"{st}={counts[kind, st]}" for st in NAMES.values()))
for row in mismatches: print("MISMATCH", row)
for row in contract[:20]: print("CONTRACT", row)
assert not mismatches and not contract
