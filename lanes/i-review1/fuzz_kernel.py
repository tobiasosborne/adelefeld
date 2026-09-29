"""i-review1: exact oracle for the real kernel of adf_idele_mul / inv / set_rat.
usage: python3 fuzz_kernel.py SEED NCASES   (runs ./kern in batches, checks with Fractions)"""
import random, subprocess, sys, os
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
KERN = os.path.join(HERE, "kern")
P_LIST = [1, 2, 3, 4, 8, 30, 64, 100, 4096]

def rnd_ball(rng, p):
    nb = rng.choice([1, 2, 3, 10, 30, p, p + 5, 2 * p + 40])
    nb = min(nb, 6000)
    M = rng.getrandbits(nb) | 1
    if rng.random() < 0.5:
        M = -M
    E = rng.randint(-20, 20) if rng.random() < 0.7 else rng.randint(-3 * p - 100, 3 * p + 100)
    m = F(M) * F(2) ** E
    mode = rng.random()
    if mode < 0.15:
        RM, RE = 0, 0
    else:
        k = rng.choice([1, 2, 3, 5, 10, 29, 30, 31, 60, 200])
        target = abs(m) * (1 - F(1, 2 ** k)) if rng.random() < 0.6 else abs(m) * F(rng.random())
        if rng.random() < 0.15:
            target = abs(m) * F(rng.choice([1, 2, 3]), 2 ** rng.randint(0, 8))
            target = min(target, abs(m) * (1 - F(1, 2 ** 40)))
        # 30-bit mantissa <= target
        if target == 0:
            RM, RE = 0, 0
        else:
            e = target.numerator.bit_length() - target.denominator.bit_length()
            RE = e - 30
            RM = int(target / F(2) ** RE)
            while RM >= 2 ** 30:
                RM //= 2; RE += 1
            if RM == 0: RM, RE = 0, 0
    r = F(RM) * F(2) ** RE if RM else F(0)
    assert abs(m) > r
    return (M, E, RM, RE), m, r


def rnd(t, p, up):
    """RD/RU of positive Fraction t to p bits"""
    e = t.numerator.bit_length() - t.denominator.bit_length()
    while F(2) ** e > t: e -= 1
    while F(2) ** (e + 1) <= t: e += 1
    # 2^e <= t < 2^(e+1); p-bit grid step 2^(e+1-p)
    step = F(2) ** (e + 1 - p)
    q = t / step
    n = q.numerator // q.denominator
    if up and n * step != t: n += 1
    return n * step

def ex(t):
    """FLINT exponent: 2^(e-1) <= t < 2^e"""
    e = t.numerator.bit_length() - t.denominator.bit_length()
    while F(2) ** e > t: e -= 1
    while F(2) ** (e + 1) <= t: e += 1
    return e + 1

def ends(m, r, p):
    return rnd(abs(m) - r, p, False), rnd(abs(m) + r, p, True)

def fr(mant, e):
    return F(mant) * (F(2) ** e)

def run(cases):
    inp = "".join(c[0] for c in cases)
    out = subprocess.run([KERN], input=inp, capture_output=True, text=True, timeout=170)
    return out.stdout.splitlines(), out.returncode, out.stderr

def parse(line):
    t = line.split()
    st = int(t[0])
    if st != 0 or len(t) < 5 or t[1].startswith("sentinel") or t[1].startswith("untouched"):
        return st, None, None, t
    mid = fr(int(t[1]), int(t[2]))
    rad = fr(int(t[3]), int(t[4]))
    return st, mid, rad, t

def main():
    seed = int(sys.argv[1]); n = int(sys.argv[2])
    rng = random.Random(seed)
    bad = 0; okc = 0; ndc = 0; maxbits = {}
    cases = []
    for i in range(n):
        p = rng.choice(P_LIST)
        pe = max(p, 2)
        op = rng.choice(["mul", "mul", "inv", "rat"])
        alias = rng.choice([0, 0, 1, 2, 3]) if op == "mul" else rng.choice([0, 1])
        if op == "mul":
            b1, m1, r1 = rnd_ball(rng, pe); b2, m2, r2 = rnd_ball(rng, pe)
            if alias == 3: b2, m2, r2 = b1, m1, r1
            txt = "mul %d %d %d %d %d %d %d %d %d %d\n" % ((alias, p) + b1 + b2)
            sx = 1 if m1 > 0 else -1; sy = 1 if m2 > 0 else -1
            l1, h1 = ends(m1, r1, pe); l2, h2 = ends(m2, r2, pe); tlo = (abs(m1) - r1) * (abs(m2) - r2); thi = (abs(m1) + r1) * (abs(m2) + r2); lo = rnd(l1 * l2, pe, False); hi = rnd(h1 * h2, pe, True); s = sx * sy
            cases.append((txt, ("mul", pe, s, lo, hi, txt, tlo, thi)))
        elif op == "inv":
            b1, m1, r1 = rnd_ball(rng, pe)
            txt = "inv %d %d %d %d %d %d\n" % ((alias, p) + b1)
            s = 1 if m1 > 0 else -1
            l1, h1 = ends(m1, r1, pe); tlo = 1 / (abs(m1) + r1); thi = 1 / (abs(m1) - r1); lo = rnd(1 / h1, pe, False); hi = rnd(1 / l1, pe, True)
            cases.append((txt, ("inv", pe, s, lo, hi, txt, tlo, thi)))
        else:
            num = rng.getrandbits(rng.choice([1, 5, 30, 100, 300])) * rng.choice([1, -1]) or 1
            den = rng.getrandbits(rng.choice([1, 5, 30, 100, 300])) | 1
            if rng.random() < 0.3: den = 2 ** rng.randint(0, 100)
            q = F(num, den)
            txt = "rat 0 %d %d %d\n" % (p, num, den)
            cases.append((txt, ("rat", pe, 1 if q > 0 else -1, rnd(abs(q), pe, False), rnd(abs(q), pe, True), txt, abs(q), abs(q))))
    B = 400
    for k in range(0, n, B):
        chunk = cases[k:k + B]
        lines, rc, err = run(chunk)
        if rc != 0 or len(lines) != len(chunk):
            print("HARNESS/ABORT rc=%d lines=%d/%d stderr=%s" % (rc, len(lines), len(chunk), err[:300]))
            # bisect
            for c in chunk:
                l, rc2, e2 = run([c])
                if rc2 != 0 or not l:
                    print("  CRASH case:", c[0].strip(), "rc", rc2, e2[:200]); bad += 1
            continue
        for c, line in zip(chunk, lines):
            op, pe, s, lo, hi, txt, tlo, thi = c[1]
            st, mid, rad, t = parse(line)
            zone_ok = ex(hi) - ex(lo) <= pe
            zone_nd = ex(hi) - ex(lo) > pe
            if st == 0:
                okc += 1
                if mid is None: print("BAD parse", txt, line); bad += 1; continue
                a, b = mid - rad, mid + rad
                if not (a <= s * tlo and s * thi <= b) and not (a <= s * thi and s * tlo <= b):
                    print("ENCLOSURE", txt.strip(), line); bad += 1
                if a <= 0 <= b or (mid > 0) != (s > 0):
                    print("SIGN/ZERO", txt.strip(), line); bad += 1
                if zone_nd and op != "rat":
                    print("OK BUT B1 SAYS ND", txt.strip(), line); bad += 1
                if op == "rat" and rad != 0 and False: pass
                if op != "rat":
                    bits = int(t[1]).bit_length()
                    maxbits[pe] = max(maxbits.get(pe, -99999), bits - (2 * pe + 30))
            elif st == 5 or st == 6 or True:
                if op == "rat":
                    print("RAT NOT OK", txt.strip(), line); bad += 1; continue
                ndc += 1
                if zone_ok:
                    print("ND BUT B1 SAYS OK", txt.strip(), line); bad += 1
                if not line.endswith("=1"):
                    print("OUTPUT NOT UNTOUCHED", txt.strip(), line); bad += 1
                # status number
                if ndc == 1: print("ND status value:", st)
    print("seed", seed, "n", n, "OK", okc, "ND", ndc, "bad", bad, "max(midbits-(2p+30))", maxbits)

main()
