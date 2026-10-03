import subprocess, random, re
from mpmath import mp, mpf, log
mp.dps = 60
random.seed(7)
import sys
SHRINK = len(sys.argv) > 1
bad = 0; n = 0
for _ in range(400):
    mid = mpf(random.choice([1, -1])) * mpf(10) ** random.randint(-250, 250) * random.uniform(1, 9.99)
    rel = mpf(10) ** random.randint(-8, -1)
    rad = abs(mid) * rel
    mids = mp.nstr(mid, 15, min_fixed=-1000, max_fixed=1000)
    rads = mp.nstr(rad, 3)
    prec = random.choice([10, 30, 80])
    idl = "(%s +/- %s ; 3 * [1 mod 4])" % (mids, rads)
    for cmd in ["Log", "log_abs"]:
        inp = "prec %d\ndigits 15\n%s %s\nLog_at %s with real\nlog_abs_at %s with real\n" % (prec, cmd, idl, idl, idl)
        r = subprocess.run(["lanes/f-review10/w/adf"], input=inp, capture_output=True, text=True, timeout=30)
        outs = r.stdout.strip().split("\n")
        # first output line is the cmd result (adele): "(a +/- b ; 0 mod 4)"
        for line, which in zip(outs, [cmd, "Log_at", "log_abs_at"]):
            n += 1
            if line.startswith("error"):
                if mid > 0 or which in ("log_abs_at", "log_abs") and False:
                    print("ERR", line, idl, which); bad += 1
                elif mid < 0 and which in ("log_abs", "log_abs_at"):
                    print("ERR neg abs", line, idl, which); bad += 1
                continue
            if mid < 0 and which in ("Log", "Log_at"):
                print("neg accepted", line, idl); bad += 1; continue
            m = re.search(r"(?:\(|real: )(-?[0-9.e+-]+)(?: \+/- ([0-9.e+-]+))?", line)
            c = mpf(m.group(1)); w = mpf(m.group(2)) if m.group(2) else 0
            lo, hi = mpf(mids) - mpf(rads), mpf(mids) + mpf(rads)
            if SHRINK: lo, hi = lo * (1 - mpf(10)**-9), hi * (1 + mpf(10)**-9)
            vals = [log(abs(lo)), log(abs(hi))]
            tlo, thi = min(vals), max(vals)
            if not (c - w <= tlo + mpf(10)**-14 * abs(tlo) and c + w >= thi - mpf(10)**-14 * abs(thi)):
                print("NOT ENCLOSED", line, idl, which, tlo, thi); bad += 1
print("checked", n, "bad", bad)
