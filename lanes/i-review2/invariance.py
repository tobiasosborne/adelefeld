"""Class invariance under exact rational scaling, checked as finite residue sets."""
import math
from fractions import Fraction as Q
from review import run

cases = []
for n in range(1, 41):
    for c in range(1, n + 1):
        if math.gcd(c, n) == 1:
            for q in (-8, -3, -1, 1, 3, 8):
                cases.append((c, n, q))
lines = []
for c, n, q in cases:
    lines.append(f"real class -5 0 1 -2 6 35 {c} {n} 1 64 1 1\n")
    nq = abs(Q(6 * q, 35))
    cq = (c if q > 0 else -c) % n or n
    lines.append(f"real class {-5*q} 0 {abs(q)} -2 {nq.numerator} {nq.denominator} "
                 f"{cq} {n} 1 64 1 1\n")
rows = run(lines)
points = 0
for case, a, b in zip(cases, rows[::2], rows[1::2]):
    c, n, q = case
    assert int(a[0]) == int(b[0]) == 0
    ca, na = map(int, a[4:6]); cb, nb = map(int, b[4:6])
    assert na == nb
    T = math.lcm(n, 210)
    sa = {w for w in range(1, T) if math.gcd(w, T) == 1 and (w - ca) % na == 0}
    sb = {w for w in range(1, T) if math.gcd(w, T) == 1 and (w - cb) % nb == 0}
    assert sa == sb and sa
    points += len(sa)
    # (5 +/- 1/4)/(6/35) = [665/24,245/8]. Check its two endpoints in both.
    for row in (a, b):
        mid, rad = map(Q, row[1:3])
        assert mid-rad <= Q(665, 24) and mid+rad >= Q(245, 8)
print(f"invariance: cases={len(cases)} enumerated_class_units={points} failures=0")
