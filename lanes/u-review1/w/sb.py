import random, sys
import gen, ref as R
def run(seed, nbase):
    rng = random.Random(seed)
    items = []
    for _ in range(nbase):
        t = gen.valid(rng, "sball")
        toks = t.split(" ")
        tag = toks[3]
        ci = {"n": 4, "r": 8, "c": 12}[tag]
        head = toks[:ci]; n = int(toks[ci], 16); body = toks[ci + 1:]
        lbs = [body[5 * i:5 * i + 5] for i in range(n)]
        def mk(lbs, cnt=None):
            return " ".join(head + [gen.hx(len(lbs) if cnt is None else cnt)] + [x for l in lbs for x in l]).encode()
        items.append((t.encode(), None, "l"))
        if n >= 1:
            i = rng.randrange(n)
            items.append((mk(lbs[:i] + [lbs[i]] + lbs[i:]), None, "l"))
            items.append((mk(lbs[:i] + lbs[i + 1:]), None, "l"))
            items.append((mk(lbs, n - 1), None, "l"))
            items.append((mk(lbs, n + 1), None, "l"))
            items.append((mk(lbs + [lbs[0]]), None, "l"))
            items.append((mk(lbs[::-1]), None, "l"))
            for _ in range(3):
                j = rng.randrange(n); lb = list(lbs[j])
                lb[0] = rng.choice(["4", "0", "1", "ffffffffffffffff", "10000000000000000", "-3", "ffffffffffffffc5", "9", "f", "1b"])
                items.append((mk(lbs[:j] + [lb] + lbs[j + 1:]), None, "l"))
            for lim in [(10**7, 1, 1, n), (10**7, 1, 1, n - 1), (10**7, 1, 1, 0), (10**7, 1, 3, 1048576), (10**7, 1, 100000, 1)]:
                items.append((t.encode(), lim, "l"))
        items.append((" ".join(head + [gen.hx(1 << 70)]).encode(), None, "l"))
        items.append((" ".join(head + [gen.hx(1 << 70)]).encode(), (10**7, 1, 1, 5), "l"))
        items.append((" ".join(head + ["5"]).encode(), (10**7, 1, 1, 2), "l"))
        items.append((" ".join(head + ["3"] + body[:5]).encode(), (10**7, 1, 1, 2), "l"))
        for tg in "nrcx":
            items.append((" ".join(toks[:3] + [tg] + toks[4:]).encode(), None, "l"))
        if tag == "c":
            items.append((" ".join(toks[:3] + ["r"] + toks[4:8] + toks[12:]).encode(), None, "l"))
    return gen.check("sball", items)
run(int(sys.argv[1]), int(sys.argv[2]))
