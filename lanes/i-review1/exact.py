"""i-review1: header sentences: exact q with <= p bits gives an exact ball; exact inputs with exact result fitting
in p bits give an exact result (radius 0, midpoint the true product)."""
import random, subprocess, os
from fractions import Fraction as F
HERE = os.path.dirname(os.path.abspath(__file__))
rng = random.Random(5)
lines = []; exp = []
for _ in range(20000):
    p = rng.choice([1, 2, 3, 5, 30, 64, 200])
    pe = max(p, 2)
    kind = rng.choice(["rat", "mul", "inv"])
    if kind == "rat":
        n = rng.getrandbits(rng.randint(1, pe)) | 1
        if rng.random() < .5: n = -n
        e = rng.randint(-200, 200)
        q = F(n) * F(2) ** e
        num, den = q.numerator, q.denominator
        lines.append("rat 0 %d %d %d\n" % (p, num, den)); exp.append((kind, q))
    elif kind == "mul":
        a = rng.getrandbits(rng.randint(1, pe)) | 1; b = rng.getrandbits(rng.randint(1, pe)) | 1
        if (a * b).bit_length() > pe: continue
        if rng.random() < .5: a = -a
        if rng.random() < .5: b = -b
        ea, eb = rng.randint(-100, 100), rng.randint(-100, 100)
        lines.append("mul %d %d %d %d 0 0 %d %d 0 0\n" % (rng.choice([0, 1, 2, 3]) if a != b else 3, p, a, ea, b, eb) if False else
                     "mul 0 %d %d %d 0 0 %d %d 0 0\n" % (p, a, ea, b, eb))
        exp.append((kind, F(a) * F(b) * F(2) ** (ea + eb)))
    else:
        a = 2 ** rng.randint(0, 50) * (1 if rng.random() < .5 else -1)
        ea = rng.randint(-100, 100)
        lines.append("inv 0 %d %d %d 0 0\n" % (p, a, ea)); exp.append((kind, 1 / (F(a) * F(2) ** ea)))
o = subprocess.run([os.path.join(HERE, "kern")], input="".join(lines), capture_output=True, text=True, timeout=120)
out = o.stdout.splitlines()
assert len(out) == len(lines), (o.returncode, len(out), len(lines), o.stderr[:200])
bad = 0
for ln, e, l in zip(lines, exp, out):
    t = l.split()
    ok = t[0] == "0" and t[3] == "0" and F(int(t[1])) * F(2) ** int(t[2]) == e[1]
    if not ok:
        bad += 1
        if bad < 10: print("NOT EXACT", ln.strip(), "->", l, "want", e[1] if e[1].denominator < 2**200 else "...")
print("exactness cases", len(lines), "bad", bad)
