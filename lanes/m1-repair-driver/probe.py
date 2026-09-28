#!/usr/bin/env python3
"""probe.py: run the cases of the repair on a driver build and print what it wrote.
    python3 lanes/m1-repair-driver/probe.py [build/adf]
Every case is a line of the script; the lines of the output are in the same order."""
import subprocess, sys
sys.set_int_max_str_digits(0)
ADF = sys.argv[1] if len(sys.argv) > 1 else "build/adf"
two = lambda k: str(2**k)
cases = [
  "show (%s ; 0)" % two(99999),
  "show (%s ; 0)" % two(100000),
  "show (0 +/- %s ; 0)" % two(99999),
  "show (0 +/- %s ; 0)" % two(100000),
  "show ((0) + (%s)*i ; 0)" % two(100000),
  "prec 100001", "div (1 ; 0) with 3",
  "prec 100002", "div (1 ; 0) with 3",
  "prec 100001", "add (1 ; 0) with 1/3",
  "prec 100002", "add (1 ; 0) with 1/3",
  "prec 100100", "div (1 ; 0) with 3",
  "prec 99000", "div (1 ; 0) with 3",
  "prec 1000000", "div (1 ; 0) with 3",
  "prec 1000001", "prec 0", "prec -1", "prec 100000", "prec 100002",
  "show (1e20000 ; 0)", "mul (1e20000 ; 0) with (1e20000 ; 0)",
  "add [5 mod 6] with 1/0",
  "add 1/0 with [5 mod 6]",
  "add [5 mod 6] with (1e100001 ; 0)",
  "reconstruct (* ; 1 mod 2) with [5 mod 6] with 1",
  "reconstruct (* ; 1 mod 2) with 0 with [5 mod 6]",
  "cap (* ; 3 mod 12) with (* ; 1 mod 2)",
  "reconstruct (* ; 1 mod 2) with (* ; 0 mod 1) with 1",
  "reconstruct (* ; 1 mod 2) with (0 ; 0) with 1",
  "prec 64\r", "show 7/3",
  "prec 64 ", "show 7/3",
  "prec\t64\t", "show 7/3",
  "prec 64\nshow 7/3",
  "compare (* ; 1 mod 2) with (* ; 3 mod 2)",
  "dump 7/3",
  "load adf1 Q rat 7 3",
]
# every case in a script of its own, so that the lines pair with the cases: a setting that
# fails writes a line and a setting that succeeds writes none
print("rc", p.returncode if False else "", "each case in a script of its own")
for i, c in enumerate(cases):
    p = subprocess.run([ADF], input=(c + "\n").encode(), capture_output=True)
    out = [l for l in p.stdout.decode("latin-1").split("\n")[:-1]]
    o = " | ".join(out) if out else "<no line>"
    cs = repr(c); cs = cs if len(cs) <= 46 else cs[:40] + "...(%d)" % len(c)
    os_ = o; os_ = os_ if len(os_) <= 90 else os_[:80] + "...(%d)" % len(o)
    print("%-48s -> %-14s %s" % (cs, "rc=%d" % p.returncode, os_))
