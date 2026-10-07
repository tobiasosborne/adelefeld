"""Hunt 5: texts from an own printer (conventions 9.2/11.3 forms as in tests/golden/ffun.tsv, rfun.tsv), dumps,
mutated dump bytes. Usage: hunt5.py N nmut seed"""
import sys, random, re
from fractions import Fraction as Fr
from m import run

N = int(sys.argv[1]); NMUT = int(sys.argv[2]); seed = int(sys.argv[3]); prog = sys.argv[4] if len(sys.argv) > 4 else "t"
rng = random.Random(seed)
fails = []
S_ = {"texts": 0, "dumps": 0, "mut": 0, "mut_ok": 0, "mut_refused": 0, "statuses": {}}


def note(k, m):
    fails.append((k, m))


def dec():
    """a decimal string and its exact value"""
    k = rng.random()
    if k < 0.15:
        return "0", Fr(0)
    ip = rng.randint(0, 99)
    nd = rng.randint(0, 4)
    fp = "".join(rng.choice("0123456789") for _ in range(nd))
    if fp and fp[-1] == "0":
        fp = fp[:-1] + "5"
    s = str(ip) + ("." + fp if fp else "")
    v = Fr(ip) + (Fr(int(fp), 10 ** len(fp)) if fp else 0)
    if rng.random() < 0.4 and v != 0:
        s, v = "-" + s, -v
    return s, v


def rad():
    if rng.random() < 0.6:
        return "", Fr(0)
    r = rng.choice(["0.25", "0.5", "0.001", "1e-5", "0.125", "3"])
    return " +/- " + r, Fr(r) if "e" not in r else Fr(1, 10 ** 5)


def cball():
    a, va = dec(); ra, rva = rad(); b, vb = dec(); rb, rvb = rad()
    return "(%s%s) + (%s%s)*i" % (a, ra, b, rb), (va, vb)


def pos_ball():
    a = rng.choice(["1", "0.5", "2", "0.125", "3.75", "10"])
    b, vb = dec()
    return "(%s) + (%s)*i" % (a, b), (Fr(a), vb)


def ffun_text():
    D, M = rng.randint(1, 5), rng.randint(1, 5)
    vs = [cball() for _ in range(D * M)]
    return "ffun(D=%d, M=%d; %s)" % (D, M, ", ".join(v[0] for v in vs)), [v[1] for v in vs]


def rfun_text():
    n = rng.randint(0, 3)
    terms, vals = [], []
    for _ in range(n):
        l = rng.randint(0, 4)
        P = [cball() for _ in range(l)]
        while P and P[-1][1] == (0, 0) and "+/-" not in P[-1][0]:
            P[-1] = cball()
        A = pos_ball()
        B, C = cball(), cball()
        terms.append("term(P=[%s], A=%s, B=%s, C=%s)" % (", ".join(p[0] for p in P), A[0], B[0], C[0]))
        vals += [p[1] for p in P] + [A[1], B[1], C[1]]
    return "rfun(%s)" % ", ".join(terms), vals


NUM = re.compile(r"\(([-0-9.e+]+)(?: \+/- ([-0-9.e+]+))?\) \+ \(([-0-9.e+]+)(?: \+/- ([-0-9.e+]+))?\)\*i")


def balls(text):
    out = []
    for m in NUM.finditer(text):
        a, ra, b, rb = m.groups()
        out.append(((Fr(a), Fr(ra) if ra else Fr(0)), (Fr(b), Fr(rb) if rb else Fr(0))))
    return out


lines, cases = [], []
for i in range(N):
    kind = "f" if i % 2 == 0 else "r"
    txt, vals = ffun_text() if kind == "f" else rfun_text()
    prec = rng.choice([20, 53, 128])
    digits = rng.choice([5, 15, 30])
    lines.append("%stext %d %d %s" % (kind, prec, digits, txt)); cases.append(("text", kind, txt, vals))
    lines.append("%sdump %s" % (kind, txt)); cases.append(("dump", kind, txt, vals))
out = run(lines, prog)
dumps = []
for (c, kind, txt, vals), line in zip(cases, out):
    st, res = line.split("\t", 1)
    if c == "text":
        S_["texts"] += 1
        if st != "0":
            note("text", "status %s for %s" % (st, txt)); continue
        bs = balls(res)
        # trailing exact zeros are not generated; values in printing order
        flat = []
        for b in bs:
            flat.append(b)
        if len(flat) != len(vals):
            note("text", "count %d vs %d: %s -> %s" % (len(flat), len(vals), txt, res)); continue
        for b, v in zip(flat, vals):
            for (m, r), x in zip(b, v):
                if abs(x - m) > r:
                    note("text", "value %s outside printed (%s +/- %s): %s -> %s" % (x, m, r, txt, res))
    else:
        if st != "0":
            note("dump", "status %s %s" % (st, txt)); continue
        dumps.append((kind, res))
# identity: load(dump) OK and redump byte-identical
lines = ["%sload %s" % (k, d) for k, d in dumps]
out = run(lines, prog)
for (k, d), line in zip(dumps, out):
    S_["dumps"] += 1
    f = line.split("\t")
    if f[0] != "0" or f[7] != d or f[2] != "0" or f[3] != "0":
        note("dump-identity", "%s -> %s" % (d, line[:300]))
# mutations
HEX = "0123456789abcdef-"


def mutate(s):
    t = list(s)
    op = rng.randint(0, 7)
    if op == 0 and t:
        t[rng.randrange(len(t))] = rng.choice(HEX + " xQ\x00\t")
    elif op == 1 and t:
        del t[rng.randrange(len(t))]
    elif op == 2:
        t.insert(rng.randrange(len(t) + 1), rng.choice(HEX + " "))
    elif op == 3:
        toks = s.split(" ")
        i = rng.randrange(len(toks))
        toks.insert(i, toks[i])
        return " ".join(toks)
    elif op == 4:
        return s[:rng.randrange(len(s))]
    elif op == 5:
        toks = s.split(" ")
        i = rng.randrange(3, len(toks)) if len(toks) > 3 else 0
        toks[i] = rng.choice(["0", "-0", "00", "1", "-1", "f" * 17, "-" + "f" * 16, "10", "7fffffffffffffff"])
        return " ".join(toks)
    elif op == 6:
        toks = s.split(" ")
        i = rng.randrange(len(toks))
        del toks[i]
        return " ".join(toks)
    else:
        return s + " 0"
    return "".join(t).replace("\x00", "")


lines, muts = [], []
while len(lines) < NMUT:
    k, d = rng.choice(dumps)
    m = mutate(d)
    if "\n" in m:
        continue
    lines.append("%sload %s" % (k, m)); muts.append((k, m))
out = run(lines, prog)
for (k, m), line in zip(muts, out):
    S_["mut"] += 1
    f = line.split("\t")
    st, un, st2, nctx, narb, nfz, nbad, red = f
    S_["statuses"][st] = S_["statuses"].get(st, 0) + 1
    if narb != "0" or nbad != "0":
        note("mut-flint", "arb_load/set_str %s bad fmpz str %s: %r" % (narb, nbad, m))
    if st != st2:
        note("mut-inspect", "load %s inspect %s: %r" % (st, st2, m))
    if st == "0":
        S_["mut_ok"] += 1
        if red != m:
            note("mut-noncanonical", "accepted %r redumped %r" % (m, red))
        if nctx != "0":
            note("mut-inspect", "nctx %s" % nctx)
    else:
        S_["mut_refused"] += 1
        if un != "U":
            note("mut-written", "status %s output written: %r" % (st, m))
        if nfz != "0":
            note("mut-fmpz", "status %s but fmpz_set_str called %s times: %r" % (st, nfz, m))
print("seed", seed, S_)
import collections
print("fails", len(fails), dict(collections.Counter(k for k, _ in fails)))
for f in fails[:30]:
    print(f)
