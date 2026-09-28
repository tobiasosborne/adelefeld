"""Independent contract cases for M1-D5, M1-D9 and alphabet/header ordering.

Expected statuses follow SPEC 15 M1-D5/M1-D9 and conventions 8.2, 8.5, 10.1, 10.2.
The original status_findings.py remains unchanged; its R2 expectations predate M1-D9.
"""
import collections
import ctypes as C
from pathlib import Path

HERE = Path(__file__).resolve().parent
lib = C.CDLL(str(HERE / "bridge.so"))


class Limits(C.Structure):
    _fields_ = [("max_len", C.c_size_t), ("max_exp10", C.c_long),
                ("max_prec", C.c_long), ("max_items", C.c_long)]


lib.review_probe.argtypes = [C.c_char_p, C.c_size_t, C.c_int, C.c_size_t,
                            C.POINTER(Limits), C.POINTER(C.c_int)]
counts = collections.Counter()
errors = []
OK, DOMAIN, UNSUPPORTED, PARSE, LIMIT = 0, 7, 8, 9, 10


def check(group, text, expected, kind=-1, items=1048576, occurrence=0, length=None):
    raw = text.encode() if isinstance(text, str) else text
    lim = Limits(1048576 if length is None else length, 100000, 100000, items)
    trouble = C.c_int()
    got = lib.review_probe(raw, len(raw), kind, occurrence, C.byref(lim), C.byref(trouble))
    counts[group] += 1
    if got != expected or trouble.value:
        errors.append((group, expected, got, trouble.value, raw[:180], len(raw)))


# Repeated blocks fail semantics at or below the cap. Above it, stage 5 wins.
# review_probe calls constructor, matching inspector and loader independently, even on failure.
for k in (65535, 65536, 65537, 65538):
    ctx = f"6 {k:x}" + " 2" * k
    fb = "l 1 " + ctx + " 0" * k
    bodies = [("modctx " + ctx, -1), ("fball " + fb, 1),
              ("scaled x 0 1 " + ctx, 2), ("scaled s 1 1 0 " + ctx, 2),
              ("adele 1 0 0 0 0 " + fb, 3), ("cadele 1 0 0 0 0 0 0 0 0 " + fb, 4),
              ("qclass lift 1 0 0 0 0 " + fb, -1),
              ("qclass pieces 1 1 0 0 0 0 " + fb, -1),
              ("qclass pieces 2 1 -1 0 0 0 g 0 1 1 1 0 0 0 0 " + fb, -1)]
    for body, kind in bodies:
        text = "adf1 Q " + body
        for items in (k - 1, k, k + 1):
            want = LIMIT if items < k else UNSUPPORTED if k > 65536 else DOMAIN
            check("R1", text, want, kind, items)
        check("R1", text + " zz", PARSE, kind)
        check("R1", text, LIMIT, kind, length=len(text)-1)


# Exact midpoint m*2^e: for m=1 it lies in [0,1] exactly when e<=0.
# An odd nonnegative radius is valid at any exponent within the decision's bound.
# For m=0 only exponent 0 is canonical. Negative m and nonzero even m fail semantics.
bound = 2**20
for field in ("mid", "rad"):
    for sign in (-1, 1):
        for magnitude in (bound-1, bound, bound+1, 2**160-1):
            exponent = sign * magnitude
            for mantissa in (0, 1, -1, 2):
                for backend in ("l 1 2 1 2 0", "g 0 2 1"):
                    if field == "mid":
                        ball = f"{mantissa:x} {exponent:x} 0 0"
                        valid = mantissa == 1 and exponent <= 0
                    else:
                        ball = f"1 -1 {mantissa:x} {exponent:x}"
                        valid = mantissa == 1
                    want = LIMIT if magnitude > bound else OK if valid else DOMAIN
                    if backend.startswith("g") and want == OK:
                        want = DOMAIN  # the valid dump has no context occurrence
                    check("R2", "adf1 Q qclass pieces 1 1 " + ball + " " + backend, want)

for exponent in (bound+1, -bound-1, 2**160-1, -(2**160-1)):
    ball = f"1 {exponent:x} 0 0"
    for bad_first in ("1 -1 0 0 0 g 0 1 1", "1 1 0 0 0 l 0 2 1 2 0"):
        text = f"adf1 Q qclass pieces 2 {bad_first} 1 {ball} l 1 2 1 2 0"
        check("R2", text, LIMIT)
        check("R2", text, LIMIT, occurrence=2**64-1)
        check("R2", text + " z", PARSE)
    for body, kind in ((f"qclass lift 1 {ball} l 1 2 1 2 0", -1),
                       (f"adele 1 {ball} l 1 2 1 2 0", 3),
                       (f"cadele 1 {ball} {ball} l 1 2 1 2 0", 4)):
        check("R2-unbounded", "adf1 Q " + body, OK, kind)

# Every byte at an internal separator and after the body, with three headers and all typed APIs.
# Alphabet errors win before unsupported headers. The known header reaches body grammar.
for byte in range(256):
    allowed = 32 <= byte <= 126 or byte in (9, 10, 13)
    for head in (b"adf1 Q ", b"adf2 Q ", b"adf1 R "):
        for body in (b"rat" + bytes([byte]) + b"1 1", b"rat 1 1" + bytes([byte])):
            if not allowed:
                expected = PARSE
            elif head != b"adf1 Q ":
                expected = UNSUPPORTED
            elif body == b"rat 1 1":
                expected = DOMAIN  # valid rational, no context occurrence
            elif body.startswith(b"rat 1 1") and chr(byte) in "0123456789abcdef":
                expected = DOMAIN  # positive hexadecimal denominator, still canonical numerator 1
            else:
                expected = PARSE
            for kind in range(5):
                # On a valid rat, the other typed entry points reject the start symbol. The
                # constructor is still DOMAIN and review_probe checks matching loader/inspector.
                check("R4", head + body, expected, kind)

for ws in (b"\t", b"\n", b"\r"):
    check("R4", b"adf1" + ws + b"Q rat 1 1", PARSE, 0)
    check("R4", b"adf1 " + ws + b" rat 1 1", PARSE, 0)
    check("R4", b"adf1 Q" + ws + b"rat 1 1", UNSUPPORTED, 0)
    check("R4", b"adf2 Q rat" + ws + b"1 1", LIMIT, 0, length=1)

for group, count in counts.items():
    print(f"{group}: cases={count}")
for error in errors:
    print("FAIL", error)
print(f"total_cases={sum(counts.values())} failures={len(errors)}")
assert not errors
