#!/usr/bin/env python3
"""Independent small grammar cases for context extraction and header precedence.

Run from the repository root with the unchanged dump_run reproducer as argument.
The expected contexts follow the token positions in conventions.md 10.1.
"""
import subprocess
import sys

binary = sys.argv[1]
cases = []


def add(s, occurrence, expected, max_items=-2):
    cases.append((s, occurrence, max_items, expected))


def one(s, K, blocks):
    spelling = "adf1 Q modctx %x %x%s" % (
        K, len(blocks), "".join(" %x" % q for q in blocks))
    add(s, 0, "OK %d %s DUMP=%s" % (K, ",".join(map(str, blocks)), spelling))
    add(s, 1, "DOMAIN")
    add(s, 2, "DOMAIN")


for K, blocks in ((6, (6,)), (6, (2, 3)), (10, (2, 5)), (1, ())):
    ctx = "%x %x%s" % (K, len(blocks), "".join(" %x" % q for q in blocks))
    one("adf1 Q modctx " + ctx, K, blocks)
    one("adf1 Q scaled x 1 1 " + ctx, K, blocks)
    if blocks:
        fb = "l 1 " + ctx + "".join(" 0" for _ in blocks)
        one("adf1 Q fball " + fb, K, blocks)
        one("adf1 Q adele 1 0 0 0 0 " + fb, K, blocks)
        one("adf1 Q cadele 1 0 0 0 0 0 0 0 0 " + fb, K, blocks)
        one("adf1 Q qclass lift 1 0 0 0 0 " + fb, K, blocks)

piece = ("adf1 Q qclass pieces 2 1 1 -3 0 0 l 1 6 1 6 0 "
         "1 7 -3 0 0 l 1 a 2 2 5 0 0")
add(piece, 0, "OK 6 6 DUMP=adf1 Q modctx 6 1 6")
add(piece, 1, "OK 10 2,5 DUMP=adf1 Q modctx a 2 2 5")
add(piece, 2, "DOMAIN")
add(piece + " ", 0, "PARSE")
add(piece[:-1] + "6", 0, "DOMAIN")  # final residue equals its block
add(piece, 0, "LIMIT", max_items=1)  # later occurrence has two blocks
add("adf1 Q rat 1 1", 0, "DOMAIN")
add("adf1 Q fball g 0 1 1", 0, "DOMAIN")
for version in ("2", "9", "10", "100", "999999999999999999999999999"):
    add("adf" + version + " Q modctx 1 0", 0, "UNSUPPORTED")
    for separator in ("x", "_", "Q", "\t"):
        add("adf" + version + separator + " Q modctx 1 0", 0, "PARSE")
add("adf1x Q modctx 1 0", 0, "PARSE")
add("adf1 Qx modctx 1 0", 0, "UNSUPPORTED")

inp = "".join("%d %d %s\n" % (occ, lim, s.encode().hex())
              for s, occ, lim, _ in cases)
run = subprocess.run([binary], input=inp.encode(), capture_output=True, check=True)
got = run.stdout.decode().splitlines()
assert len(got) == len(cases), (len(got), len(cases), run.stderr)
bad = 0
for (s, occ, lim, want), value in zip(cases, got):
    if value != want:
        bad += 1
        print("FAIL", repr(s), occ, lim, "got", value, "want", want)
print("cases", len(cases), "wrong", bad)
print(run.stderr.decode().strip())
sys.exit(bool(bad))
