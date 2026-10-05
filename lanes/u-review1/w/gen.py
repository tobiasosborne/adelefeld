import random, subprocess, sys, os
from math import gcd
sys.path.insert(0, os.path.dirname(__file__))
import ref as R

PRIMES = [2, 3, 5, 7, 11, 13, 101, 65537, (1 << 31) - 1, (1 << 61) - 1, (1 << 64) - 59]


def hx(n):
    return "0" if n == 0 else ("-" if n < 0 else "") + format(abs(n), "x")


def rbits(rng, b):
    return rng.getrandbits(max(1, b)) | (1 << (max(1, b) - 1))


def rint(rng, signed=True, maxb=4000):
    b = rng.choice([1, 2, 3, 8, 30, 31, 32, 64, 65, 128, rng.randint(1, maxb)])
    v = rbits(rng, b)
    return -v if signed and rng.random() < .4 else v


def rarb(rng, positive=False, away=False):
    r = rng.random()
    if r < .1 and not away:
        return (0, 0, 0, 0)
    m = rint(rng, not positive, 600) | 1
    if m < 0 and m % 2 == 0:
        m -= 1
    e = rng.choice([0, 0, 1, -1, 5, -5, rng.randint(-100, 100), rng.randint(-5000, 5000), rng.choice([1, -1]) * rbits(rng, rng.choice([63, 64, 65, 200]))])
    rm = rng.choice([0, 1, 3, (1 << 30) - 1, (1 << 29) + 1, rbits(rng, 30) | 1, rng.randint(1, 1000) | 1])
    re_ = 0 if rm == 0 else rng.choice([e + 1 - m.bit_length(), e - m.bit_length() - 3, e, e - 1, rng.randint(-100, 100)]) if abs(e) < 10 ** 6 else rng.randint(-5, 5)
    if rm == 0:
        re_ = 0
    return (m, e, rm, re_)


def arbt(a):
    return " ".join(hx(x) for x in a)


def rprime(rng):
    if rng.random() < .6:
        return rng.choice(PRIMES)
    while True:
        p = rng.getrandbits(rng.choice([8, 16, 32, 64])) | 1
        if R.is_prime(p):
            return p


def rlb(rng, p=None):
    p = p or rprime(rng)
    if rng.random() < .5:
        while True:
            num = rint(rng, True, 300) if rng.random() < .9 else 0
            den = rint(rng, False, 300)
            g = gcd(num, den)
            num //= g or 1
            den //= g or 1
            if num == 0:
                den = 1
            if num % p and den % p or num == 0:
                break
        v = 0 if num == 0 else rng.choice([0, 1, -1, 5, -7, rng.randint(-3000, 3000)])
        return (p, "x", num, den, v)
    if rng.random() < .1:
        return (p, "b", 0, 0, rng.randint(-50, 50))
    v = rng.choice([0, 0, 1, -1, 3, rng.randint(-20, 20)])
    k = rng.randint(1, 40)
    N = v + k
    while True:
        u = rng.randint(1, max(1, min(p ** k, 1 << rng.choice([8, 64, 600])) - 1))
        if u % p and u < p ** k:
            break
        if p ** k <= 2:
            u = 1
            break
    return (p, "b", u, v, N)


def lbt(lb):
    return f"{hx(lb[0])} {lb[1]} {hx(lb[2])} {hx(lb[3])} {hx(lb[4])}"


def ruco(rng):
    if rng.random() < .15:
        return (rng.choice([1, -1]), 0)
    N = rint(rng, False, 600)
    if rng.random() < .1:
        N = rng.choice([1, 2, 3, 4, 6])
    while True:
        c = rng.randint(1, N)
        if gcd(c, N) == 1:
            return (c, N)


def valid(rng, typ):
    for _ in range(100):
        t = _valid(rng, typ)
        if R.ref(typ, t.encode()) == 0:
            return t
    raise SystemExit("no valid " + typ)


def _valid(rng, typ):
    if typ == "ucoset":
        c, N = ruco(rng)
        return f"adf1 Q ucoset {hx(c)} {hx(N)}"
    if typ == "idele":
        c, N = ruco(rng)
        q = rint(rng, False, 200)
        d = rint(rng, False, 200)
        g = gcd(q, d)
        return f"adf1 Q idele 1 {arbt(rarb(rng))} {hx(q // g)} {hx(d // g)} {hx(c)} {hx(N)}"
    if typ == "idclass":
        c, N = ruco(rng)
        return f"adf1 Q idclass {arbt(rarb(rng, True))} {hx(c)} {hx(N)}"
    if typ == "lball":
        return "adf1 Q lball " + lbt(rlb(rng))
    n = rng.choice([0, 1, 2, 3, 10, rng.randint(0, 40)])
    ps = sorted(set(rprime(rng) for _ in range(n)))
    tag = rng.choice("nrc")
    a = "n" if tag == "n" else ("r " + arbt(rarb(rng)) if tag == "r" else "c " + arbt(rarb(rng)) + " " + arbt(rarb(rng)))
    return f"adf1 Q sball {a} {hx(len(ps))}" + "".join(" " + lbt(rlb(rng, p)) for p in ps)


NASTY = ["0", "-0", "00", "01", "-1", "1", "2", "3", "f", "F", "-f", "7fffffff", "40000000", "3fffffff", "80000000",
         "ffffffff", "10000000000000000", "ffffffffffffffc5", "ffffffffffffffff", "7fffffffffffffff",
         "8000000000000000", "-8000000000000000", "-8000000000000001", "10000000000000001", "x", "b", "n", "r", "c",
         "", "-", "0x1", "1g", "+1", " ", "ffffffffffffffc4", "10000000000000000000", "1" + "0" * 1250, "-1" + "0" * 1250, "4",
         "a", "9", "7", "d", "e", "ffffffffffffffc6", "fffffffffffffffe", "100000000000000", "7ffffffffffffff"]
NAMES = ["ucoset", "idele", "idclass", "lball", "sball", "rat", "adele", "qclass", "Lball", ""]


def mutate(rng, t, typ):
    toks = t.split(" ")
    k = rng.randint(0, 29)
    i = rng.randrange(len(toks)) if toks else 0
    b = t.encode()
    if k == 0 and len(toks) > 1:
        del toks[i]
    elif k == 1:
        toks.insert(i, toks[i])
    elif k == 2 and len(toks) > 1:
        j = rng.randrange(len(toks))
        toks[i], toks[j] = toks[j], toks[i]
    elif k in (3, 4, 5, 6):
        toks[i] = rng.choice(NASTY)
    elif k in (7, 8, 9):
        try:
            v = int(toks[i], 16)
            toks[i] = hx(v + rng.choice([-2, -1, 1, 2, 1 << 30, -(1 << 30), 1 << 63, 1 << 64, -v]))
        except ValueError:
            toks[i] = rng.choice(NASTY)
    elif k == 10:
        toks[i] = "0" + toks[i]
    elif k == 11:
        toks[i] = toks[i].upper()
    elif k == 12:
        toks[i] = "-" + toks[i]
    elif k == 13:
        toks[i] = toks[i] + rng.choice(["0", "00", "x", "-", "g"])
    elif k == 14:
        sep = rng.choice(["  ", "\t", "\n", "\r", " \t"])
        s = sep.join(toks[:i + 1]) + (" " if i + 1 < len(toks) else "") + " ".join(toks[i + 1:])
        return s.encode()
    elif k == 15:
        return (t + rng.choice([" ", "\n", "\r", "\t", "\x00", "  ", "\x7f", "\xe9"])).encode("latin-1")
    elif k == 16:
        return (rng.choice([" ", "\n", "\x00"]) + t).encode("latin-1")
    elif k == 17:
        n = rng.randrange(len(b))
        return b[:n] + bytes([rng.choice([0, 9, 10, 13, 0x20, 0x7f, 0x80, 0xff, 0x7e, 0x41, 0x2d, 0x30])]) + b[n + 1:]
    elif k == 18:
        n = rng.randrange(len(b))
        return b[:n] + bytes([rng.randrange(256)]) + b[n + 1:]
    elif k == 19:
        toks[min(3, len(toks) - 1)] = rng.choice(NAMES)
    elif k == 20:
        toks[0] = rng.choice(["adf0", "adf2", "adf", "adf10", "adf01", "ADF1", "adf1x", "adf-1", "adf11"])
    elif k == 21 and len(toks) > 1:
        toks[1] = rng.choice(["q", "Qx", "R", "QQ", "", "1", "F3(T)", "Q\t"])
    elif k == 22:
        n = rng.randrange(len(b) + 1)
        return b[:n]
    elif k == 23:
        # count tweak: the token after the type name, or the count of sball
        for j in range(3, len(toks)):
            try:
                v = int(toks[j], 16)
            except ValueError:
                continue
            if typ == "sball":
                c = 4 if toks[3] == "n" else (8 if toks[3] == "r" else 12)
                j = c
                if j < len(toks):
                    try:
                        toks[j] = hx(int(toks[j], 16) + rng.choice([-1, 1, 2, 1 << 64]))
                    except ValueError:
                        pass
            else:
                try:
                    toks[4] = hx(int(toks[4], 16) + rng.choice([-1, 1, 2]))
                except ValueError:
                    pass
            break
    elif k == 24:
        # make mantissa even / radius mantissa 30,31 bits: poke any token to even or wide
        v = rng.choice([2, 4, 6, 0x20000000, 0x3fffffff, 0x40000000, 0x7fffffff, 0x80000001, 0xfffffffe])
        toks[i] = hx(v)
    elif k == 25:
        toks = toks[:i] + toks[i + 1:] + [toks[i]]
    elif k == 26:
        toks[i] = rng.choice(["x", "b", "n", "r", "c"])
    elif k == 27:
        toks[i] = hx(rng.choice([1 << 62, 1 << 63, -(1 << 63), (1 << 63) - 1, (1 << 64) - 1, 1 << 64, -(1 << 64), 1 << 5000, -(1 << 5000), (1 << 64) - 59, 4, 9, 1, 0]))
    else:
        toks = toks + [rng.choice(NASTY)]
    return " ".join(toks).encode("latin-1")


def limit_for(rng, n):
    r = rng.random()
    if r < .8:
        return None
    return rng.choice([(n, 100000, 100000, 1048576), (max(n - 1, 0), 100000, 100000, 1048576), (0, 1, 1, 1), (10 ** 7, 0, 0, 0),
                       (10 ** 7, 100000, 1, 1), (10 ** 7, 100000, 0, 1048576), (10 ** 7, 1, 100000, 0), (10 ** 7, 1, 5, 3),
                       (10 ** 7, 1, 1 << 62, 1048576), (10 ** 7, 1, (1 << 63) - 1, 10 ** 7), (10 ** 7, 1, 100, 1)])


def run(lines, san=False, timeout=170):
    exe = os.path.join(os.path.dirname(__file__), os.environ.get("FZ", "fz-san" if san else "fz"))
    env = dict(os.environ, NOFORK=("1" if san else ""), ASAN_OPTIONS="detect_leaks=1:abort_on_error=0", UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1")
    p = subprocess.run([exe], input="\n".join(lines) + "\n", capture_output=True, text=True, timeout=timeout, env=env)
    return p.stdout.split("\n")[:-1], p.stderr, p.returncode


def main(typ, nbase, seed, san=False, nmut=60, cuts=True):
    rng = random.Random(seed)
    items = []   # (text bytes, limits, mode)
    for _ in range(nbase):
        t = valid(rng, typ)
        items.append((t.encode(), None, "l"))
        for _ in range(nmut):
            items.append((mutate(rng, t, typ), None, "l"))
        if cuts and rng.random() < .03:
            b = t.encode()
            for n in range(len(b) + 1):
                items.append((b[:n], None, "l"))
        for _ in range(3):
            b = t.encode()
            items.append((b, limit_for(rng, len(b)) or (len(b), 10 ** 5, 10 ** 5, 10 ** 6), "l"))
        items.append((t.encode(), None, rng.choice("abc")))
    # attach limits to some mutated
    items2 = []
    for b, lim, m in items:
        if lim is None and rng.random() < .15:
            lim = limit_for(rng, len(b))
        items2.append((b, lim, m))
    return check(typ, items2, san)


def check(typ, items, san=False, show=12):
    lines = []
    for b, lim, m in items:
        l = "- - - -" if lim is None else " ".join(str(x) for x in lim)
        lines.append(f"{typ} {m} {l} {b.hex() if b else '-'}")
    out, err, rc = run(lines, san)
    bad = {}
    nok = 0
    if rc != 0 or len(out) != len(lines):
        print("HARNESS FAILURE rc", rc, len(out), len(lines), err[-2000:]); print("CRASH LINE", lines[len(out)][:600] if len(out)<len(lines) else None)
        return
    for (b, lim, m), o in zip(items, out):
        if o.startswith('CRASH'):
            bad.setdefault('PROCESS ' + o, []).append((b, lim or R.DEF, m, 'crash ' + o)); continue
        f = o.split()
        st, canon, deq, ist, nctx, un = (int(x) for x in f)
        L = lim or R.DEF
        rexp = R.ref(typ, b, L)
        exp = rexp
        if m in "bc" and exp == 0:
            exp = R.DOMAIN
        why = None
        if st != exp:
            why = f"status C={st} ref={exp}"
        elif st == 0:
            nok += 1
            if canon != 1:
                why = "OK but not canonical"
            elif deq != 1:
                why = "dump != text"
        elif un != 1:
            why = "output changed on non-OK"
        if why is None:
            if ist != rexp:
                why = f"inspect status {ist} != {rexp}"
            elif nctx != (0 if rexp == 0 else 77):
                why = f"nctx {nctx} for status {rexp}"
        if why:
            bad.setdefault(why.split(" C=")[0] if why.startswith("status") else why, []).append((b, L, m, why))
    print(f"{typ}: {len(items)} cases, {nok} OK, mismatches {sum(len(v) for v in bad.values())}")
    for k, v in bad.items():
        print(" ", k, len(v))
        seen = set()
        for b, L, m, why in sorted(v, key=lambda x: len(x[0]))[:show]:
            print("    ", why, m, L if L != R.DEF else "", repr(b[:200]))
    return bad


if __name__ == "__main__":
    typ = sys.argv[1]
    main(typ, int(sys.argv[2]), int(sys.argv[3]), san=len(sys.argv) > 4 and sys.argv[4] == "san")
