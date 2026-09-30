import sys, subprocess, time
lines = open(sys.argv[1]).read().splitlines()
lim = float(sys.argv[2])
for i, l in enumerate(lines):
    t = time.time()
    try:
        r = subprocess.run(["/tmp/x/drv"], input=l + "\n", capture_output=True, text=True, timeout=lim)
        dt = time.time() - t
        if dt > 0.5:
            print(i, "%.2fs" % dt, "prec", l.split()[0], "deg", len(l.split()) - 2, r.stdout[:30], flush=True)
    except subprocess.TimeoutExpired:
        print(i, "TIMEOUT >%ss" % lim, "prec", l.split()[0], "coeffs", l[:300], flush=True)
