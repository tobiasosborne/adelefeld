#!/usr/bin/env python3
"""hostile.py: hostile inputs for the driver adf; each case prints what the driver wrote.

    python3 docs/reviews/m1/surface/checks/hostile.py [ADF]

Run from the repository root.  Every case gives the bytes, the expected behaviour with its
source, and what the driver did.  A case whose behaviour differs from the expectation is
marked MISMATCH.  Cases are run under the named binary (default build/adf); run once more
with build/adf-san for the sanitizers, and under valgrind with ADF="valgrind -q build/adf".
"""
import shlex, subprocess, sys, time
sys.set_int_max_str_digits(0)

ADF = shlex.split(sys.argv[1]) if len(sys.argv) > 1 else ["build/adf"]
mism = 0


def run(data, extra=()):
    t = time.time()
    p = subprocess.run(ADF + list(extra), input=data, capture_output=True, timeout=170)
    return p.returncode, p.stdout, p.stderr, time.time() - t


def case(name, data, want_lines, why, show_err=False):
    global mism
    rc, out, err, dt = run(data)
    got = out.decode("latin-1").split("\n")[:-1]
    ok = want_lines is None or got == want_lines
    if not ok:
        mism += 1
    short = [g if len(g) < 100 else g[:60] + "...(%d bytes)" % len(g) for g in got]
    print("%-4s %-38s rc=%d %.2fs %s" % ("ok" if ok else "MISMATCH", name, rc, dt, short))
    if not ok:
        print("       want %s  -- %s" % (want_lines, why))
    if err:
        print("       stderr:", err.decode("latin-1")[:400])


two100000 = str(2 ** 100000)          # 30103 digits; the value is exactly 2^100000
two99999 = str(2 ** 99999)
assert len(two100000) < 65000

# ---- the guard on the binary exponent (SPEC 15.2, M1-D1: "refuses to print a real ball whose
#      binary exponent exceeds 100000 in absolute value")
case("guard: mid 2^99999", ("show (%s ; 0)\n" % two99999).encode(), ["(4.9950104650719225397e30102 +/- 2.1e30082 ; 0)"],
     "binary exponent 99999 (x = 1.0 * 2^99999): printed")
case("guard: mid 2^100000", ("show (%s ; 0)\n" % two100000).encode(), ["(9.9900209301438450794e30102 +/- 4.1e30082 ; 0)"],
     "binary exponent 100000 does not exceed 100000 (x = 1.0 * 2^100000): printed (M1-D1)")
case("guard: rad 2^100000", ("show (0 +/- %s ; 0)\n" % two100000).encode(), ["(0 +/- 1e30103 ; 0)"],
     "radius 2^100000, binary exponent 100000: printed (M1-D1)")
case("guard: mid 2^100000 in cadele imag", ("show ((0) + (%s)*i ; 0)\n" % two100000).encode(),
     ["((0) + (9.9900209301438450794e30102 +/- 4.1e30082)*i ; 0)"], "as above, in the imaginary part")

# ---- the line length: at most 65536 bytes (tools/adf/README.md, line limit)
line = b"show 7/3" + b" " * (65536 - 8)
case("65536 bytes + CRLF", line[:-1] + b"\r\n", ["7/3"], "65536 bytes, CR is the last of them")
case("65536 bytes then 65537", line + b"\n" + line + b" \nshow 1/2\n", ["7/3", "error: LIMIT", "1/2"],
     "README: a longer line is error: LIMIT and the rest of it is skipped")

# ---- the separator
case("sep at start", b"add with 1\n", ["error: PARSE"], "one operand 'with 1': wrong arity")
case("sep at end", b"add 1 with \n", ["error: PARSE"], "one operand")
case("sep twice", b"add 1 with  with 2\n", ["error: PARSE"], "'with 2' is not a value text")
case("sep with tabs", b"add 1\twith\t2\n", ["error: PARSE"], "the separator is ' with ' (M1-D1)")
case("whitespace operand", b"add \t with 2\n", ["error: PARSE"], "")
case("four operands", b"add 1 with 2 with 3 with 4 with 5\n", ["error: PARSE"], "arity")
case("uppercase WITH", b"add 1 WITH 2\n", ["error: PARSE"], "")
case("trailing CR on value", b"add 7/3 with 1/3\r\n", ["8/3"], "CR is whitespace of the value form (conv. 8.2)")

# ---- settings: argument with trailing whitespace or CR (CRLF script)
case("prec 64 CRLF", b"prec 64\r\nshow 7/3\r\n", [ "7/3"],
     "a CRLF script: every value command accepts the CR (conv. 8.2); the setting line too?")
case("prec 64 trailing blank", b"prec 64 \nshow 7/3\n", ["7/3"],
     "README: 'a command may be written with spaces around it'")
case("prec leading zeros", b"prec 0000000000000000000064\nshow (1 ; 0)\n", ["(1 ; 0)"], "decimal integer")
case("prec huge", b"prec 99999999999999999999999999\n", ["error: DOMAIN"], "out of range")
case("prec -0", b"prec -0\n", ["error: DOMAIN"], "")
case("prec +5", b"prec +5\n", ["error: PARSE"], "not a decimal integer")

# ---- NUL and high bytes
case("NUL in op word", b"sh\x00ow 7/3\n", ["error: PARSE"], "")
case("NUL after sep", b"add 1 with \x002\n", ["error: PARSE"], "")
case("NUL only", b"\x00\n", ["error: PARSE"], "")
case("VT as blank", b"show\x0b7/3\n", ["error: PARSE"], "only space and tab are blanks")

# ---- reconstruct: operands 2 and 3 are not read by the value parser of the driver
case("reconstruct lo unsupported", b"reconstruct (* ; 1 mod 2) with [5 mod 6] with 1\n", ["error: UNSUPPORTED"],
     "README: UNSUPPORTED 'is decided before every other check'")
case("reconstruct hi unsupported", b"reconstruct (* ; 1 mod 2) with 0 with [5 mod 6]\n", ["error: UNSUPPORTED"],
     "README: UNSUPPORTED 'is decided before every other check'")
case("reconstruct lo fball", b"reconstruct (* ; 1 mod 2) with (* ; 0 mod 1) with 1\n", ["error: DOMAIN"],
     "README: 'Every other pair is error: DOMAIN' (cap with a finite ball as C is DOMAIN)")
case("reconstruct lo adele", b"reconstruct (* ; 1 mod 2) with (0 ; 0) with 1\n", ["error: DOMAIN"],
     "as above")

# ---- long digit strings
big = "9" * 65000
case("65000-digit rational", ("show %s\n" % big).encode(), None, "time only")
case("mul two 32000-digit fballs", ("mul (* ; %s mod %s) with (* ; %s mod %s)\n"
     % ("7" * 16000, "3" * 16000, "5" * 16000, "2" * 16000)).encode(), None, "time only")
case("65000-digit decimal in adele", ("show (%s.5 ; 0)\n" % ("1" * 64000)).encode(), None, "time only")
case("prec 1e6 digits 1e6 div", b"prec 1000000\ndigits 1000000\ndiv (1 ; 0) with 3\n", None, "time only")
case("1e100000 at prec 1e6", b"prec 1000000\nshow (1e100000 ; 0)\n", ["error: LIMIT"], "guard")
case("1e-30000 mid", b"show (1e-30000 ; 0)\n", None, "binary exponent about -99658: printed")
case("1e-30103 mid", b"show (1e-30103 ; 0)\n", None, "binary exponent about -100000")
case("1e-30104 mid", b"show (1e-30104 ; 0)\n", ["error: LIMIT"], "binary exponent about -100003: refused")
print("mismatches:", mism)
