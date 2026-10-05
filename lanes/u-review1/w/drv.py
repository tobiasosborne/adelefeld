import random, subprocess, sys, os
import gen, ref as R
root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
exe = os.path.join(root, "build", "adf-san")
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1", UBSAN_OPTIONS="print_stacktrace=1")


def drv(lines, timeout=160):
    p = subprocess.run([exe], input="\n".join(lines) + "\n", capture_output=True, text=True, timeout=timeout, env=env)
    return p.stdout.split("\n")[:-1], p.stderr, p.returncode


def main(seed, nbase):
    rng = random.Random(seed)
    rep = {}
    total = 0
    for typ in ["ucoset", "idele", "idclass", "lball", "sball"]:
        # 1: load of valid dumps and 100 malformed lines each
        bases = [gen.valid(rng, typ) for _ in range(nbase)]
        lines = []
        for t in bases:
            lines.append("load " + t)
        out, err, rc = drv(lines)
        total += len(lines)
        if rc not in (0, 1) or "ERROR" in err or "runtime error" in err or len(out) != len(lines):
            rep.setdefault("load valid failure", []).append((typ, rc, err[:400], len(out), len(lines)))
        # 2: dump of the printed value
        l2 = []
        vals = []
        for t, o in zip(bases, out):
            if o.startswith("error"):
                rep.setdefault("load of a valid dump: " + o, []).append(t[:150])
            else:
                vals.append((t, o))
                l2.append("dump " + o)
        out2, err, rc = drv(l2)
        total += len(l2)
        if "ERROR" in err or "runtime error" in err or len(out2) != len(l2):
            rep.setdefault("dump of printed value failure", []).append((typ, rc, err[:600], l2[:2]))
        else:
            same = 0
            for (t, o), o2 in zip(vals, out2):
                if o2 == t:
                    same += 1
                else:
                    rep.setdefault(f"{typ}: dump(load(D)) != D", []).append((t[:100], o[:100], o2[:100]))
            print(typ, "load->dump round trips equal:", same, "of", len(vals))
        # 3: malformed
        l3 = []
        for t in bases[:max(1, nbase // 4)]:
            for _ in range(8):
                b = gen.mutate(rng, t, typ)
                try:
                    s = b.decode("latin-1")
                except Exception:
                    continue
                if "\n" in s or "\r" in s or "\x00" in s or s != s.strip(" \t") or "\t" in s:
                    continue
                if typ in ("idele", "idclass") and ("-" in s.split(" ")[4:5] or (len(s.split(" ")) > 6 and s.split(" ")[3 + (2 if typ == "idele" else 1) - 1].startswith("-") and s.split(" ")[3 + (2 if typ == "idele" else 1) + 1] != "0")):
                    continue
                l3.append("load " + s)
        out3, err, rc = drv(l3)
        total += len(l3)
        if "ERROR" in err or "runtime error" in err or len(out3) != len(l3) or rc not in (0, 1):
            rep.setdefault("malformed load failure", []).append((typ, rc, err[:800], l3[:1]))
        else:
            # compare with reference: error statuses
            names = {7: "DOMAIN", 8: "UNSUPPORTED", 9: "PARSE", 10: "LIMIT"}
            mism = 0
            for l, o in zip(l3, out3):
                txt = l[5:].encode("latin-1")
                exp = R.ref(typ, txt)
                if exp == 0 and o.startswith("error") and o != "error: LIMIT":
                    mism += 1
                    rep.setdefault("driver errors on a loadable text", []).append((l[:150], o))
                elif exp != 0 and not o.startswith("error: " + names[exp]):
                    # the driver reads the kind from the dump header; other types have other loaders.
                    if o.startswith("error"):
                        rep.setdefault(f"driver status differs from ref ({typ})", []).append((l[:150], o, names[exp]))
                    else:
                        rep.setdefault(f"driver accepts text the ref refuses ({typ})", []).append((l[:150], o, names[exp]))
            print(typ, "malformed lines:", len(l3), "mismatches", mism)
    print("total lines", total)
    for k, v in rep.items():
        print("FINDING?", k, len(v))
        for x in v[:6]:
            print("   ", x)


main(int(sys.argv[1]), int(sys.argv[2]))
