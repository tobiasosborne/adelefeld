"""f-repair4: classify the failures of the reviewer's N-D13 oracle on the N-D14 build. A failure is EXPECTED when it
is an exponent mismatch of an OK result for a ball input with requested N < E and the returned exponent is N
(the line format of h: MODE p unum uden v N exact n seed Nreq cap). Prints counts; any other failure is listed."""
import re, sys
exp_ok = other = 0
for ln in sys.stdin:
    if not ln.startswith('FAIL'): continue
    m = re.search(r'exponent (-?\d+), exact image (-?\d+) \| (\w) \S+ \S+ \S+ \S+ (-?\d+) (\d) \S+ \S+ (-?\d+)', ln)
    if m:
        got, E, mode, Nball, exact, Nreq = int(m[1]), int(m[2]), m[3], int(m[4]), int(m[5]), int(m[6])
        if exact == 0 and Nreq < E and got == Nreq:
            exp_ok += 1; continue
    other += 1; print("UNEXPECTED", ln.rstrip()[:300])
print("N-D13 failures explained by N-D14 (ball, Nreq < E, exponent Nreq):", exp_ok, "; other:", other)
