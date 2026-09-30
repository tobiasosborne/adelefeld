import sys; sys.set_int_max_str_digits(0)
import subprocess, time
from flint import fmpz
M = 16777216
n = int(sys.argv[1]); mode = sys.argv[2]
p = fmpz(2) ** n
if mode == 'lin':      # 2^n X - 1 : root 2^-n
    line = "2 -1 %s\n" % p
elif mode == 'lin3':   # (2^n X - 1) with extra: 2^n X - 3 : root 3 2^-n
    line = "2 -3 %s\n" % p
elif mode == 'quad':   # (2^n X - 1)(X - 1) = 2^n X^2 - (2^n + 1) X + 1
    line = "2 1 %s %s\n" % (-(p + 1), p)
elif mode == 'neg':    # 2^n X + 1
    line = "2 1 %s\n" % p
elif mode == 'big':    # X - 2^n : root 2^n
    line = "2 %s 1\n" % (-p)
t = time.time()
r = subprocess.run(["/tmp/x/drv"], input=line, capture_output=True, text=True, timeout=float(sys.argv[3]))
print("n=M%+d" % (n - M), mode, "%.2fs" % (time.time() - t), "rc", r.returncode, r.stdout[:120].strip(), r.stderr[:200])
