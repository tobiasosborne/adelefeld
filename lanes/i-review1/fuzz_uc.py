"""i-review1: unit cosets against enumeration in (Z/L)^*, L = lcm(moduli) * q with q a prime >= 5 not dividing
the lcm, so that every coset with N >= 1 is strictly larger than an exact unit at level L."""
import subprocess, os, sys, random
from math import gcd

HERE = os.path.dirname(os.path.abspath(__file__))
UC = os.path.join(HERE, "uc")

def lcm(a, b): return a * b // gcd(a, b) if a and b else (a or b)

def level(*Ns):
    l = 1
    for n in Ns:
        if n: l = lcm(l, n)
    for q in (5, 7, 11, 13, 17):
        if l % q: return l * q
    raise

def units(L): return [u for u in range(L) if gcd(u, L) == 1]

def cset(v, L, U):
    c, N = v
    if N == 0: return frozenset([c % L])
    return frozenset(u for u in U if (u - c) % N == 0)

def run(lines, timeout=170):
    o = subprocess.run([UC], input="".join(lines), capture_output=True, text=True, timeout=timeout)
    return o.returncode, o.stdout.splitlines(), o.stderr

def canon_list(maxN):
    out = [(1, 0), (-1, 0)]
    for N in range(1, maxN + 1):
        for c in range(1, N + 1):
            if gcd(c, N) == 1: out.append((c, N))
    return out

def main():
    maxN = int(sys.argv[1]); Lmax_mul = int(sys.argv[2]); seed = int(sys.argv[3]); nsamp = int(sys.argv[4])
    rng = random.Random(seed)
    vals = canon_list(maxN)
    byN = {}
    for v in vals: byN.setdefault(v[1], []).append(v)
    Ns = sorted(byN)
    # pairs: all pairs of moduli, sampled cosets
    cases = []
    for _ in range(nsamp):
        N1 = rng.choice(Ns); N2 = rng.choice(Ns)
        cases.append((rng.choice(byN[N1]), rng.choice(byN[N2])))
    # plus exhaustive small
    small = [v for v in vals if v[1] <= 12]
    for a in small:
        for b in small: cases.append((a, b))
    bad = 0; n = 0; nm = 0
    Ucache = {}
    def U(L):
        if L not in Ucache: Ucache[L] = units(L)
        return Ucache[L]
    lines = []; exp = []
    for (x, y) in cases:
        L = level(x[1], y[1])
        if L > 60000: continue
        Uu = U(L)
        A = cset(x, L, Uu); B = cset(y, L, Uu)
        for op, want in (("contains", int(A <= B)), ("eq", int(A == B)), ("ov", int(bool(A & B)))):
            lines.append("%s %d %d %d %d\n" % (op, x[0], x[1], y[0], y[1]))
            exp.append(("bool", op, x, y, want))
        if L <= Lmax_mul:
            P = frozenset((a * b) % L for a in A for b in B)
            lines.append("mul %d %d %d %d\n" % (x[0], x[1], y[0], y[1]))
            exp.append(("set", "mul", x, y, P, L))
            nm += 1
    for x in vals:
        L = level(x[1])
        if L > 60000: continue
        Uu = U(L); A = cset(x, L, Uu)
        Iv = frozenset(pow(a, -1, L) for a in A)
        lines.append("inv %d %d\n" % x); exp.append(("set1", "inv", x, Iv, L))
        lines.append("norm %d %d\n" % x); exp.append(("set1", "norm", x, A, L))
    rc, out, err = run(lines)
    if rc != 0 or len(out) != len(lines):
        print("ABORT rc", rc, len(out), len(lines), err[:300]); return
    for e, line in zip(exp, out):
        n += 1
        t = line.split()
        if e[0] == "bool":
            _, op, x, y, want = e
            if int(t[0]) != want:
                print("WRONG", op, x, y, "code", t[0], "true", want); bad += 1
        else:
            if e[0] == "set":
                _, op, x, y, P, L = e; res = (int(t[0]), int(t[1]))
            else:
                _, op, x, P, L = e; res = (int(t[0]), int(t[1])); y = None
            if len(t) > 2:
                print("FLAG", op, x, y, line); bad += 1
            Uu = U(L)
            if cset(res, L, Uu) != P:
                print("WRONG SET", op, x, y, "code", res); bad += 1
    print("checks", n, "mul enumerations", nm, "bad", bad)

main()
