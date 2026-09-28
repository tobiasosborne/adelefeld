#!/usr/bin/env python3
"""Check neighbouring inputs of surface R7 to R11 against M1-D1 and M1-D6.

Run from the repository root after building build/adf. No expected text is taken from adf.
"""
import subprocess
import sys

sys.set_int_max_str_digits(0)


def check(name, data, expected, prefix=False):
    p = subprocess.run(["build/adf"], input=data.encode(), capture_output=True, check=False)
    got = p.stdout.decode().splitlines()
    good = (got[:len(expected)] == expected if prefix else got == expected)
    print(f"{name}: exit {p.returncode}, lines {len(got)}, {'PASS' if good else 'FAIL'}")
    if not good:
        print("  got:", [x[:90] for x in got])
        print("  want:", expected)
    return good


one_below = str(2 ** 99999)
one_above = str(2 ** 100000)
halfway = str(2 ** 51000)
cases = [
    ("mid admitted", f"show ({one_below} ; 0)\n", ["("], True),
    ("mid refused", f"show ({one_above} ; 0)\n", ["error: LIMIT"], False),
    ("radius admitted", f"show (0 +/- {one_below} ; 0)\n", ["(0 +/- "], True),
    ("radius refused", f"show (0 +/- {one_above} ; 0)\n", ["error: LIMIT"], False),
    ("imaginary refused", f"show ((0) + ({one_above})*i ; 0)\n", ["error: LIMIT"], False),
    ("prec boundary", "prec 100000\nprec 100001\nprec 0\n", ["error: LIMIT", "error: DOMAIN"], False),
    ("digits CRLF", "digits 21\r\nshow 7/3\r\n", ["7/3"], False),
    ("digits blank", "digits 21 \nshow 7/3\n", ["7/3"], False),
    ("syntax before kind", "add [5 mod 6] with @\n", ["error: PARSE"], False),
    ("kind before value", "add [5 mod 6] with 1/0\n", ["error: UNSUPPORTED"], False),
    ("result guard", f"prec 100000\nshow ({halfway} ; 0)\nmul ({halfway} ; 0) with "
     f"({halfway} ; 0)\n", ["(" , "error: LIMIT"], True),
]

bad = 0
for name, data, expected, prefix in cases:
    if name in ("mid admitted", "radius admitted"):
        p = subprocess.run(["build/adf"], input=data.encode(), capture_output=True, check=False)
        got = p.stdout.decode().splitlines()
        good = p.returncode == 0 and len(got) == 1 and got[0].startswith(expected[0])
        print(f"{name}: exit {p.returncode}, lines {len(got)}, {'PASS' if good else 'FAIL'}")
    elif name == "result guard":
        p = subprocess.run(["build/adf"], input=data.encode(), capture_output=True, check=False)
        got = p.stdout.decode().splitlines()
        good = p.returncode == 1 and len(got) == 2 and got[0].startswith("(") and got[1] == "error: LIMIT"
        print(f"{name}: exit {p.returncode}, lines {len(got)}, {'PASS' if good else 'FAIL'}")
    else:
        good = check(name, data, expected, prefix)
    bad += not good
print(f"driver_closure: {len(cases)} cases, {bad} failures")
raise SystemExit(bool(bad))
