#!/usr/bin/env python3
"""fuzz_diff.py: random commands through the driver, checked against oracle.py.

    python3 docs/reviews/m1/surface/checks/fuzz_diff.py [N] [SEED] [ADF]

Run from the repository root.  N commands (default 20000) of the operations add, sub, mul,
neg, div, equal, contains, overlaps, cap, show, over the four types rat, fball, adele,
cadele, with small random rationals and decimals.  The whole script is one driver run.

Checks:
  * rat and fball results, predicates, statuses: the exact text of the oracle;
  * adele and cadele results: the finite part exactly; each real coordinate as an
    enclosure: the printed interval [M - R, M + R] contains the exact interval of the
    oracle (conventions 9.5: the printed interval contains the arb, which contains the
    true set).  When the oracle's real value is exact and a dyadic number with a short
    mantissa, the text itself is compared too (conventions 9.5 'tightness' for dyadic
    input and exact dyadic arithmetic at 64 bits).
"""
import os, random, re, shlex, subprocess, sys
from fractions import Fraction as F
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(__file__))
import oracle as O

N = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
SEED = int(sys.argv[2]) if len(sys.argv) > 2 else 1
ADF = shlex.split(sys.argv[3]) if len(sys.argv) > 3 else ["build/adf"]
rng = random.Random(SEED)


def rrat(allow_neg=True, small=False):
    n = rng.randint(-30 if allow_neg else 0, 30)
    d = rng.choice([1, 1, 1, 2, 3, 4, 5, 6, 8, 9, 12, 25])
    if small:
        d = rng.choice([1, 2, 4])
    return F(n, d)


def rtxt(x):
    return O.q(x)


def rdec():
    """a decimal text and its exact value; sometimes with a radius"""
    m = F(rng.randint(-400, 400), rng.choice([1, 2, 4, 8, 10, 100]))
    ms = O.fmt(m) if O.is_decimal(m) else None
    if ms is None:
        m = F(round(m * 100), 100)
        ms = O.fmt(m)
    if rng.random() < 0.3:
        r = F(rng.randint(1, 50), rng.choice([1, 8, 10, 100]))
        return ms + " +/- " + O.fmt(r), O.Real(m - r, m + r)
    return ms, O.Real(m, m)


def rfin():
    a = rrat()
    if rng.random() < 0.25:
        return rtxt(a), O.FB(a)
    Nn = F(rng.randint(1, 36), rng.choice([1, 1, 1, 2, 3, 4, 6]))
    return rtxt(a) + " mod " + rtxt(Nn), O.FB(a, Nn)


def rval():
    k = rng.choice(["rat", "rat", "fball", "fball", "adele", "cadele"])
    if k == "rat":
        x = rrat()
        return rtxt(x), ("rat", x)
    ft, fv = rfin()
    if k == "fball":
        return "(* ; " + ft + ")", ("fball", fv)
    rt, rv = rdec()
    if k == "adele":
        return "(" + rt + " ; " + ft + ")", ("adele", rv, fv)
    it, iv = rdec()
    return "((" + rt + ") + (" + it + ")*i ; " + ft + ")", ("cadele", rv, iv, fv)


def encl(printed, exact):
    M, R = O.parse_real_text(printed)
    return M - R <= exact.lo and exact.hi <= M + R


def dyadic_short(x):
    if not x.exact():
        return False
    d = x.lo.denominator
    return d & (d - 1) == 0 and abs(x.lo.numerator) < 2 ** 40


cmds, want = [], []
ops = ["add", "sub", "mul", "neg", "div", "equal", "contains", "overlaps", "cap", "show"]
for _ in range(N):
    op = rng.choice(ops)
    if op in ("neg", "show"):
        t, v = rval()
        cmds.append("%s %s" % (op, t))
        want.append((op, [v]))
        continue
    t1, v1 = rval()
    if op in ("div", "cap") and rng.random() < 0.7:
        y = rrat()
        t2, v2 = rtxt(y), ("rat", y)
    else:
        t2, v2 = rval()
    cmds.append("%s %s with %s" % (op, t1, t2))
    want.append((op, [v1, v2]))

p = subprocess.run(ADF, input="\n".join(cmds) + "\n", capture_output=True, text=True)
got = p.stdout.split("\n")[:-1]
assert len(got) == len(cmds), (len(got), len(cmds))

REALRE = r"(-?[0-9.e-]+(?: \+/- [0-9.e-]+)?)"
bad = 0
counts = {"exact-text": 0, "enclosure": 0, "enclosure+text": 0}
for c, (op, vs), g in zip(cmds, want, got):
    e = O.run(c)
    if e is None:
        print("oracle cannot model:", c)
        continue
    v = None
    if op in ("add", "sub", "mul", "neg", "div", "show"):
        if op == "neg":
            v = O.neg(vs[0])
        elif op == "show":
            v = vs[0]
        elif op == "div":
            v = O.div(vs[0], vs[1])
        else:
            v = O.binop(op, vs[0], vs[1])
    if v is None or isinstance(v, str) or v[0] in ("rat", "fball"):
        if e != g:
            bad += 1
            print("DIFF %s\n  driver: %s\n  oracle: %s" % (c, g, e))
        counts["exact-text"] += 1
        continue
    if v[0] == "adele":
        m = re.fullmatch(r"\(" + REALRE + r" ; (.*)\)", g)
        reals, fin = ([m.group(1)], m.group(2)) if m else (None, None)
        ex = [v[1]]
        ftxt = v[2].text_fin()
    else:
        m = re.fullmatch(r"\(\(" + REALRE + r"\) \+ \(" + REALRE + r"\)\*i ; (.*)\)", g)
        reals, fin = ([m.group(1), m.group(2)], m.group(3)) if m else (None, None)
        ex = [v[1], v[2]]
        ftxt = v[3].text_fin()
    if m is None:
        bad += 1
        print("UNPARSED %s\n  driver: %s\n  oracle: %s" % (c, g, e))
        continue
    ok = fin == ftxt and all(encl(r, x) for r, x in zip(reals, ex))
    kind = "enclosure"
    def dy(val):
        if val[0] == "rat":
            return dyadic_short(O.Real.pt(val[1]))
        if val[0] == "fball":
            return True
        reals_in = val[1:-1]
        return all(r.lo.denominator & (r.lo.denominator - 1) == 0
                   and r.hi.denominator & (r.hi.denominator - 1) == 0 for r in reals_in)
    if ok and op != "div" and all(dy(w) for w in vs) and all(dyadic_short(x) for x in ex):
        kind = "enclosure+text"
        ok = all(r == O.r_print(x.lo, 0) for r, x in zip(reals, ex))
    counts[kind] += 1
    if not ok:
        bad += 1
        print("BAD %s\n  driver: %s\n  oracle: %s" % (c, g, e))
print("commands %d, failures %d, %s, exit status %d" % (len(cmds), bad, counts, p.returncode))
sys.exit(1 if bad else 0)
