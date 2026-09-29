import random, sys, os
sys.argv = ["x", "0", "0"]
HERE = os.path.dirname(os.path.abspath(__file__))
exec(open(os.path.join(HERE, "fuzz_kernel.py")).read().replace("\nmain()\n", "\n"))
rng = random.Random(9)
with open(os.path.join(HERE, "vg_kern.txt"), "w") as f:
    for _ in range(600):
        p = rng.choice([1, 2, 3, 64, 300]); pe = max(p, 2)
        a = rng.choice([0, 1, 2, 3]); b1, _, _ = rnd_ball(rng, pe); b2, _, _ = rnd_ball(rng, pe)
        k = rng.choice(["mul", "mul", "inv", "rat"])
        if k == "mul": f.write("mul %d %d %d %d %d %d %d %d %d %d\n" % ((a, p) + b1 + b2))
        elif k == "inv": f.write("inv %d %d %d %d %d %d\n" % ((a % 2, p) + b1))
        else: f.write("rat 0 %d %d %d\n" % (p, rng.randint(-99, 99), rng.randint(1, 99)))
with open(os.path.join(HERE, "vg_uc.txt"), "w") as f:
    for _ in range(600):
        N = rng.choice([0, 1, 2, 4, 6, 12, 30, 2**70 + 1, 2**200]); N2 = rng.choice([0, 1, 2, 3, 4, 6, 30])
        def cc(N):
            if N == 0: return rng.choice([1, -1])
            while True:
                c = rng.randint(1, N)
                if __import__("math").gcd(c, N) == 1: return c
        op = rng.choice(["mul", "inv", "norm", "contains", "eq", "ov", "set"])
        if op in ("inv", "norm"): f.write("%s %d %d\n" % (op, cc(N), N))
        elif op == "set": f.write("set %d %d\n" % (rng.randint(-50, 50), rng.choice([-1, 0, 1, 2, 4, 6, 9])))
        else: f.write("%s %d %d %d %d\n" % (op, cc(N), N, cc(N2), N2))
