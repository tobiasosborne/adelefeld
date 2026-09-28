"""Reproduce the constructor cap bypass and time large dumps within default text limits."""
import ctypes as C
import math
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
marks = bytearray(b"\1") * 1100000
marks[:2] = b"\0\0"
for p in range(2, math.isqrt(len(marks)-1)+1):
    if marks[p]:
        marks[p*p::p] = b"\0" * ((len(marks)-1-p*p)//p+1)
primes = [p for p, yes in enumerate(marks) if yes]
lib = C.CDLL(str(HERE / "bridge.so"))
lib.review_inspect.argtypes = [C.c_char_p, C.c_size_t, C.c_void_p]
lib.review_inspect.restype = C.c_int

def text(qs, body):
    return (f"adf1 Q {body} {math.prod(qs):x} {len(qs):x} " +
            " ".join(format(q, "x") for q in qs)).encode()

for count, square in [(65537, 0), (65536, 32768), (83354, 0)]:
    qs = [p*p if i < square else p for i, p in enumerate(primes[:count])]
    dump = text(qs, "modctx")
    assert len(qs) == count and len(dump) <= 1048576
    path = HERE / "large_dump.txt"
    path.write_bytes(dump)
    scaled = text(qs, "scaled x 0 1")
    start = time.process_time()
    st = lib.review_inspect(scaled, len(scaled), None)
    print(f"count={count} squared_prefix={square} inspect_status={st} "
          f"inspect_seconds={time.process_time()-start:.6f}", flush=True)
    result = subprocess.run([str(HERE / "bridge"), str(path)], capture_output=True, text=True, timeout=150)
    print(result.stdout.strip(), flush=True)
    print(f"exit={result.returncode}", flush=True)
    if result.stderr: print(result.stderr, flush=True)
    path.unlink()
