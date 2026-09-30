import sys; sys.set_int_max_str_digits(0)
from oracle import *
ins = open('/tmp/x/vg_in.txt').read().splitlines(); outs = open('/tmp/x/vg_out3.txt').read().splitlines()
nb = 0
for a, o in zip(ins, outs):
    t = a.split(); prec = int(t[0]); co = [int(x) for x in t[1:]]
    bad = check_one(prec, co, o)
    if bad: nb += 1; print(bad, o[:80])
print("valgrind inputs", len(ins), "outputs", len(outs), "defects", nb)
