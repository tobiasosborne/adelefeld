import random, sys
import gen, ref as R
typ = "idele"
rng = random.Random(int(sys.argv[1]))
items = []
skipped = 0
for _ in range(int(sys.argv[2])):
    t = gen.valid(rng, typ)
    cands = [t.encode()] + [gen.mutate(rng, t, typ) for _ in range(60)]
    for b in cands:
        toks = b.split(b" ")
        if len(toks) > 6 and toks[4].startswith(b"-") and toks[6] != b"0":
            skipped += 1
            continue
        items.append((b, None, "l"))
print("skipped (negative mid, nonzero radius):", skipped)
gen.check(typ, items, san=True)
