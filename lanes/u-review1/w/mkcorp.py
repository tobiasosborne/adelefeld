import random, sys
import gen, ref as R
rng = random.Random(int(sys.argv[1]))
nl, ns = int(sys.argv[2]), int(sys.argv[3])
big = R.DEF[:2] + (1 << 63 - 1 and (1 << 63) - 1, 1 << 30)
out = []
for _ in range(nl):
    out.append("lball " + gen.valid(rng, "lball")[len("adf1 Q lball "):] if False else "lball " + gen.valid(rng, "lball"))
for _ in range(ns):
    out.append("sball " + gen.valid(rng, "sball"))
EXT = [0, 1, -1, 1 << 30, -(1 << 30), 1 << 60, -(1 << 60), (1 << 60) + 1, 1 << 61, -(1 << 61), 1 << 62, -(1 << 62), (1 << 63) - 1, -(1 << 63), (1 << 63) - 2, -(1 << 63) + 1]
for p in (2, 3, 5, (1 << 64) - 59):
    for a in EXT:
        out.append(f"lball adf1 Q lball {gen.hx(p)} x 1 1 {gen.hx(a)}" if p not in (2,) else f"lball adf1 Q lball 2 x 1 1 {gen.hx(a)}")
        out.append(f"lball adf1 Q lball {gen.hx(p)} x 1 3 {gen.hx(a)}" if p != 3 else f"lball adf1 Q lball 3 x 1 2 {gen.hx(a)}")
        out.append(f"lball adf1 Q lball {gen.hx(p)} b 0 0 {gen.hx(a)}")
        for b in EXT:
            if a < b:
                out.append(f"lball adf1 Q lball {gen.hx(p)} b 1 {gen.hx(a)} {gen.hx(b)}")
                out.append(f"lball adf1 Q lball {gen.hx(p)} b {gen.hx(rng.randint(1, 1 << 40) | 1)} {gen.hx(a)} {gen.hx(b)}")
out = [o for o in out if o.startswith(("lball ", "sball "))]
for l in range(len(out)):
    t = out[l].split(" ", 1)[1]
    out[l] = out[l].split(" ", 1)[0] + " " + t
# sball with extreme lbs
for _ in range(300):
    ps = sorted(set(rng.choice([2, 3, 5, 7, 11]) for _ in range(rng.randint(1, 4))))
    lbs = []
    for p in ps:
        a = rng.choice(EXT)
        lbs.append(f"{gen.hx(p)} x 1 1 {gen.hx(a)}" if rng.random() < .5 else f"{gen.hx(p)} b 0 0 {gen.hx(a)}")
    tag = rng.choice("nrc")
    arb = "n" if tag == "n" else ("r " + gen.arbt(gen.rarb(rng)) if tag == "r" else "c " + gen.arbt(gen.rarb(rng)) + " " + gen.arbt(gen.rarb(rng)))
    out.append(f"sball adf1 Q sball {arb} {gen.hx(len(ps))} " + " ".join(lbs))
sys.stdout.write("\n".join(out) + "\n")
