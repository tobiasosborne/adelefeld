"""Neighboring decimal-exponent cases for R4 and M1-D7."""
import ctypes as C
import json
from pathlib import Path

root = Path(__file__).resolve().parents[5]
lib = C.CDLL(str(root / "build-closure/bridge.so"))


class Limits(C.Structure):
    _fields_ = [("max_len", C.c_size_t), ("max_exp10", C.c_long),
                ("max_prec", C.c_long), ("max_items", C.c_long)]


lib.review_status.argtypes = [C.c_int, C.c_char_p, C.c_size_t, C.POINTER(Limits)]
cases = [
    ("(0e1000000000000000000 ; 0)", 10**18, 0),
    ("(0e-1000000000000000000 ; 0)", 10**18, 0),
    ("(0e1000000000000000001 ; 0)", 10**18, 10),
    ("(0 +/- 0e1000000000000000000 ; 0)", 10**18, 0),
    ("(0e000000000000000000000000000 ; 0)", 0, 0),
    ("(1e0 ; 0)", 0, 0),
    ("(1e1 ; 0)", 0, 10),
    ("(1e100 ; 0)", 100, 0),
    ("(1e-100 ; 0)", 100, 0),
    ("(1e101 ; 0)", 100, 10),
    ("((0e1000000000000000000) + (0)*i ; 0)", 10**18, 0),
]
for text, bound, want in cases:
    data = text.encode()
    lim = Limits(1048576, bound, 100000, 1048576)
    kind = 3 if text.startswith("((") else 2
    got = lib.review_status(kind, data, len(data), C.byref(lim))
    assert got == want, (text, bound, got, want)
print(json.dumps({"cases": len(cases), "failed": 0}))
