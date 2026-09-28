"""Small public-interface reproducers for the undocumented exponent limit and stage ordering."""
import ctypes as C
from pathlib import Path

lib = C.CDLL(str(Path("/home/tobias/Projects/adelefeld-wt/m1-repair-dump/lanes/m1-repair-dump/checks") / "bridge.so"))
lib.review_probe.argtypes = [C.c_char_p, C.c_size_t, C.c_int, C.c_size_t, C.c_void_p, C.POINTER(C.c_int)]
names = {0: "OK", 7: "DOMAIN", 8: "UNSUPPORTED", 9: "PARSE", 10: "LIMIT"}
cases = [
    ("piece within undocumented cap", "adf1 Q qclass pieces 1 1 1 -100000 0 0 l 1 2 1 2 0", "OK"),
    ("piece above undocumented cap", "adf1 Q qclass pieces 1 1 1 -100001 0 0 l 1 2 1 2 0", "OK"),
    ("invalid denominator and large exponent",
     "adf1 Q qclass pieces 1 1 1 -100001 0 0 l 0 2 1 2 0", "DOMAIN"),
    ("invalid midpoint and large exponent",
     "adf1 Q qclass pieces 1 1 -1 -100001 0 0 l 1 2 1 2 0", "DOMAIN"),
    ("unknown version with tab in body", "adf2 Q rat\t1 1", "UNSUPPORTED"),
    ("unknown field with newline in body", "adf1 R rat\n1 1", "UNSUPPORTED"),
    ("unknown version with forbidden NUL", "adf2 Q rat\x001 1", "PARSE"),
]
for label, text, expected in cases:
    b = text.encode()
    trouble = C.c_int()
    actual = lib.review_probe(b, len(b), -1, 0, None, C.byref(trouble))
    print(f"{label}: expected={expected} actual={names[actual]} untouched_errors={trouble.value} input={text!r}")
