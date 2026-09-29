"""i-review1: scale invariance of the kernel at exponents near +-2^60..2^63. Same case with mantissas and radii
shifted by 2^s must give the same result shifted, and the same status."""
import random, subprocess, os, sys
sys.argv = ["x", "0", "0"]
HERE = os.path.dirname(os.path.abspath(__file__))
exec(open(os.path.join(HERE, "fuzz_kernel.py")).read().replace("\nmain()\n", "\n"))
rng = random.Random(77)
SH = [2**60, -2**60, 2**61 + 12345, -(2**61) - 999, 2**62 - 5000, -(2**62) + 5000, 2**62 + 7, -(2**62) - 7,
      3 * 2**60, 2**63 - 2**20, -(2**63) + 2**20, 1, -1, 2**40]

def runk(t):
    o = subprocess.run([KERN], input=t, capture_output=True, text=True, timeout=100)
    return o.returncode, o.stdout, o.stderr

def sh(b, s):
    return (b[0], b[1] + s, b[2], b[3] + s if b[2] else 0)

def fits(*vs):
    return all(-2**63 < v < 2**63 - 1 for v in vs)

bad = n = nok = 0
for i in range(3000):
    p = rng.choice(P_LIST); pe = max(p, 2)
    s1 = rng.choice(SH); s2 = rng.choice(SH)
    b1, m1, r1 = rnd_ball(rng, pe); b2, m2, r2 = rnd_ball(rng, pe)
    if rng.random() < .3:
        base = "inv 0 %d %d %d %d %d\n" % ((p,) + b1)
        c1 = sh(b1, s1)
        if not fits(c1[1], c1[3]): continue
        big = "inv 0 %d %d %d %d %d\n" % ((p,) + c1)
        exp_shift = -s1
    else:
        base = "mul 0 %d %d %d %d %d %d %d %d %d\n" % ((p,) + b1 + b2)
        c1, c2 = sh(b1, s1), sh(b2, s2)
        if not fits(c1[1], c1[3], c2[1], c2[3]): continue
        big = "mul 0 %d %d %d %d %d %d %d %d %d\n" % ((p,) + c1 + c2)
        exp_shift = s1 + s2
    rc0, o0, e0 = runk(base); rc1, o1, e1 = runk(big)
    n += 1
    if rc0 or rc1:
        print("CRASH", big.strip(), rc1, e1[:200]); bad += 1; continue
    t0, t1 = o0.split(), o1.split()
    if t0[0] != t1[0]:
        print("STATUS DIFF", big.strip(), t0, t1); bad += 1; continue
    if t0[0] == "0":
        nok += 1
        m0, e0_, r0, re0 = int(t0[1]), int(t0[2]), int(t0[3]), int(t0[4])
        mm, ee, rr, rre = int(t1[1]), int(t1[2]), int(t1[3]), int(t1[4])
        if not (m0 == mm and r0 == rr and ee - e0_ == exp_shift and (r0 == 0 or rre - re0 == exp_shift)):
            print("SCALE DIFF", big.strip(), t0, t1, exp_shift); bad += 1
print("scale cases", n, "ok-status", nok, "bad", bad)
