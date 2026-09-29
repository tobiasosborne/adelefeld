"""Independent review oracle: Fraction arithmetic and finite point hulls, no author reference import."""
from fractions import Fraction as Q
from collections import Counter
import random
import subprocess
import sys

INF = 10**100
COUNTS = Counter()
BIN = sys.argv[1] if len(sys.argv) > 1 else "lanes/f-review2/bridge"
proc = subprocess.Popen(["timeout", "150", BIN], stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE, text=True, bufsize=1)


def vp(q, p):
    if not q:
        return INF
    a, b, w = abs(q.numerator), q.denominator, 0
    while a % p == 0:
        a //= p
        w += 1
    while b % p == 0:
        b //= p
        w -= 1
    return w


def exact(p, c):
    v = 0 if not c else vp(c, p)
    return (p, 1, c / Q(p)**v, v, 0)


def ball(p, c, n):
    v = vp(c, p)
    if v >= n:
        return (p, 0, Q(0), 0, n)
    t = c / Q(p)**v
    mod = p**(n-v)
    u = t.numerator * pow(t.denominator, -1, mod) % mod
    return (p, 0, Q(u), v, n)


def center(x):
    return x[2] * Q(x[0])**x[3]


SENTINEL = (7, 1, Q(11), 0, 0)


def call(op, x, y=SENTINEL, alias=0):
    line = f"{op} {alias} " + " ".join(str(t) for t in x+y) + "\n"
    proc.stdin.write(line)
    proc.stdin.flush()
    reply = proc.stdout.readline().split()
    assert len(reply) == 9, (line, reply, proc.poll())
    st, p, e, u, v, n, m, b, rat = reply[0], *reply[1:]
    # Nine tokens: status + five fields + two scalar outputs + rational output.
    return st, (int(p), int(e), Q(u), int(v), int(n)), int(m), int(b), Q(rat)


def points(x, digits):
    c = center(x)
    return [c] if x[1] else [c + Q(x[0])**x[4] * a for a in digits]


def check_result(op, x, y, digits):
    result = call(op, x, y)
    st, z = result[:2]
    want_status = "OK"
    if op in ("inv", "div"):
        d = x if op == "inv" else y
        if d[2] == 0:
            want_status = "NOT_UNIT" if d[1] else "UNIT_NOT_CERTIFIED"
    assert st == want_status, (op, x, y, result, want_status)
    COUNTS["arithmetic_cases"] += 1
    if st != "OK":
        assert z == SENTINEL
        COUNTS["status_untouched"] += 1
        return
    xx, yy = points(x, digits), points(y, digits)
    if op == "neg":
        rs = [-a for a in xx]
    elif op == "inv":
        rs = [1/a for a in xx]
    else:
        fn = {"add": lambda a, b: a+b, "sub": lambda a, b: a-b,
              "mul": lambda a, b: a*b, "div": lambda a, b: a/b}[op]
        rs = [fn(a, b) for a in xx for b in yy]
    COUNTS["point_results"] += len(rs)
    c = center(z)
    assert all(r == c if z[1] else vp(r-c, x[0]) >= z[4] for r in rs), (op, x, y, z)
    # A smallest ball containing the sampled results has exponent equal to the minimum
    # valuation of a difference from the first result. Two different next-digit classes suffice.
    hull = min((vp(r-rs[0], x[0]) for r in rs), default=INF)
    assert (hull == INF and z[1]) or (not z[1] and hull == z[4]), (op, x, y, z, hull)
    COUNTS["tight_hulls"] += 1
    assert call("canon", z)[3] == 1
    if COUNTS["arithmetic_cases"] % 13 == 0:
        for alias in (1, 2):
            got = call(op, x, y, alias)
            assert got == result, (op, alias, result, got)
            COUNTS["alias_calls"] += 1


def predicate_cases(x, y):
    p, c, d = x[0], center(x), center(y)
    sameprime = p == y[0]
    eq = sameprime and x == y
    dv = vp(c-d, p)
    over = sameprime and (c == d if x[1] and y[1]
                         else dv >= min(t[4] for t in (x, y) if not t[1]))
    inside = sameprime and ((x[1] and c == d) if y[1]
                           else (x[1] or x[4] >= y[4]) and dv >= y[4])
    for op, want in (("eq", eq), ("ident", eq), ("over", over), ("inside", inside)):
        assert call(op, x, y)[3] == want, (op, x, y, want)
        COUNTS["predicate_calls"] += 1


def run():
    rng = random.Random(912071)
    for p, count in ((2, 300), (3, 300), (5, 150), (7, 100), (2**64-59, 200)):
        digits = list(range(p**3)) if p <= 3 else (list(range(p**2)) if p <= 7
                 else [0, 1, 2, p-1, p, p+1, p*p-1])
        pool = []
        for _ in range(50):
            c = Q(rng.randint(-30, 30), rng.randint(1, 17)) * Q(p)**rng.randint(-4, 4)
            n = rng.randint(-5, 7)
            e = rng.randrange(3) == 0
            x = exact(p, c) if e else ball(p, c, n)
            pool.append(x)
            raw = (p, int(e), c, 0, n)
            assert call("construct", raw)[1] == x
            COUNTS["constructors"] += 1
            assert call("center", x)[4] == center(x)
            assert call("canon", x)[3] == 1
            assert call("cz", x)[3] == (x[2] == 0)
            assert call("exact", x)[3] == x[1]
            st, _, m, inf, _ = call("val", x)
            if x[2] == 0 and not x[1]:
                assert (st, m, inf) == ("NOT_DETERMINED", 987, 986)
                assert call("abs", x)[0::4] == ("NOT_DETERMINED", Q(13))
                assert call("dec", x)[:3] == ("NOT_DETERMINED", SENTINEL, 987)
            else:
                assert (st, m, inf) == ("OK", x[3], int(x[2] == 0))
                assert call("abs", x)[4] == (Q(p)**(-x[3]) if x[2] else 0)
                dec = call("dec", x)
                if not x[2]:
                    assert dec[:3] == ("DOMAIN", SENTINEL, 987)
                else:
                    expected = (p, x[1], x[2], 0, 0 if x[1] else x[4]-x[3])
                    assert dec[:3] == ("OK", expected, x[3])
                    assert call("dec", x, alias=1) == dec
            COUNTS["accessor_sets"] += 1
        # Explicit zeros guarantee exact/ball distinction and inversion failure cases.
        pool += [exact(p, Q(0))] + [ball(p, Q(0), n) for n in (-4, 0, 4)]
        for _ in range(count):
            x, y = rng.choice(pool), rng.choice(pool)
            predicate_cases(x, y)
            for op in ("add", "sub", "mul", "div"):
                check_result(op, x, y, digits)
        for x in pool:
            for op in ("neg", "inv"):
                check_result(op, x, SENTINEL, digits)
        print(f"prime={p} pairs={count} digits={len(digits)} cumulative={dict(COUNTS)}", flush=True)
    print("FINAL", dict(COUNTS), flush=True)


if __name__ == "__main__":
    try:
        run()
    finally:
        proc.stdin.close()
        assert proc.wait(timeout=5) == 0
