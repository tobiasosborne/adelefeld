import random, sys
from math import gcd
sys.path.insert(0, "lanes/f-review12")
from orc import run
def main(n, seed):
    rnd = random.Random(seed); cases = []
    for _ in range(n):
        N = rnd.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 15, 16, 18, 20, 24, 30, 36, 60, 64, 100, rnd.randint(1, 120)])
        n_ = rnd.choice([-3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 24, 30, 36, 60, 100, rnd.randint(1, 130)])
        if N == 0: c = rnd.choice([1, -1])
        else:
            c = rnd.randint(-50, 200)
            if gcd(c, N) != 1: continue
        cases.append((c, N, n_))
    o1 = run([f"cy {c} {N} {n}" for c, N, n in cases])
    o2 = run([f"cyi {c} {N} {n}" for c, N, n in cases])
    nf = 0; ok = 0; nd = 0
    for (c, N, n), l1, l2 in zip(cases, o1, o2):
        if n <= 0: exp = None; dom = True
        else:
            dom = False
            if N == 0: S = {c % n}
            else:
                L = N * n // gcd(N, n)
                S = {b % n for b in range(1, L + 1) if gcd(b, L) == 1 and (b - c) % N == 0}
            exp = S
        s1, j1 = int(l1.split()[0]), int(l1.split()[1]); s2, j2 = int(l2.split()[0]), int(l2.split()[1])
        def fl(*a):
            nonlocal nf
            nf += 1
            if nf < 30: print("FAIL", *a)
        if dom:
            if s1 != 7 or s2 != 7 or j1 != -777 or j2 != -777: fl("dom", c, N, n, l1, l2)
            continue
        if len(exp) == 1:
            ok += 1
            j = next(iter(exp))
            inv = pow(j, -1, n) if n > 1 else 0
            if (s1, j1) != (0, j) or (s2, j2) != (0, inv): fl("value", c, N, n, l1, l2, j, inv)
        else:
            nd += 1
            if s1 != 1 or s2 != 1 or j1 != -777 or j2 != -777: fl("nd", c, N, n, l1, l2, sorted(exp)[:5])
    print("cyclo cases", len(cases), "determined", ok, "not-determined", nd, "fails", nf)
main(int(sys.argv[1]), int(sys.argv[2]))
