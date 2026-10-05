"""three-way: my reference vs proto/text_grammar.py (dump_roundtrip), same corpora as gen.py."""
import random, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "proto"))
import gen, ref as R
import text_grammar as TG
names = {"OK": 0, "DOMAIN": 7, "UNSUPPORTED": 8, "PARSE": 9, "LIMIT": 10}
tot = 0
dis = {}
for typ in ["ucoset", "idele", "idclass", "lball", "sball"]:
    rng = random.Random(77)
    n = 0
    for _ in range(int(sys.argv[1])):
        t = gen.valid(rng, typ)
        for b in [t.encode()] + [gen.mutate(rng, t, typ) for _ in range(40)]:
            n += 1
            mine = R.ref(typ, b)
            try:
                s = b.decode("latin-1")
                o = TG.dump_roundtrip(s if all(ord(c) < 128 for c in s) else b)
            except Exception as e:
                o = "!EXC " + type(e).__name__
            if o.startswith("!"):
                theirs = names.get(o[1:], o)
            else:
                theirs = 0
                if o.encode() != b:
                    theirs = "NOT-BYTE-EQUAL"
            if mine != theirs:
                dis.setdefault((typ, mine, theirs), []).append(b[:140])
    tot += n
    print(typ, n, "cases")
print("total", tot, "disagreements", sum(len(v) for v in dis.values()))
for k, v in dis.items():
    print(k, len(v), v[:3])
