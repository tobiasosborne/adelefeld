"""Slow-case search: valid 65536-block contexts, nearly filling the default one-MiB input budget."""
import math
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
marks = bytearray(b"\1") * 821642
marks[:2] = b"\0\0"
for p in range(2, math.isqrt(len(marks)-1)+1):
    if marks[p]: marks[p*p::p] = b"\0" * ((len(marks)-1-p*p)//p+1)
primes = [p for p, yes in enumerate(marks) if yes]
assert len(primes) == 65536

def product_tree(qs):
    # This is an exact product; grouping avoids repeated multiplication by a growing accumulator.
    layer = [math.prod(qs[i:i+128]) for i in range(0, len(qs), 128)]
    while len(layer) > 1:
        layer = [math.prod(layer[i:i+2]) for i in range(0, len(layer), 2)]
    return layer[0]

def dump(prefix, quotient):
    qs = [p*p if i < prefix else p for i, p in enumerate(primes)]
    head = "adf1 Q qclass pieces 1 1 1 -1 0 0 l 1" if quotient else "adf1 Q modctx"
    text = f"{head} {product_tree(qs):x} 10000 " + " ".join(format(q, "x") for q in qs)
    if quotient: text += " 0" * len(qs)
    return text.encode()

for quotient in (False, True):
    if len(sys.argv) > 1 and ("qclass" if quotient else "modctx") != sys.argv[1]:
        continue
    lo, hi = 0, 65536
    while lo < hi:
        mid = (lo+hi+1)//2
        if len(dump(mid, quotient)) <= 1048576: lo = mid
        else: hi = mid-1
    text = dump(lo, quotient)
    path = HERE / "large_dump.txt"
    path.write_bytes(text)
    print(f"body={'qclass' if quotient else 'modctx'} squared_prefix={lo} "
          f"bytes={len(text)} next_prefix_bytes={len(dump(lo+1, quotient))}", flush=True)
    result = subprocess.run([str(HERE / "bridge"), str(path)], capture_output=True, text=True, timeout=150)
    print(result.stdout.strip(), flush=True)
    print(f"exit={result.returncode}", flush=True)
    if result.stderr: print(result.stderr, flush=True)
    path.unlink()
