"""Independent expectations and selected old golden files. No driver-suite invocation."""
from pathlib import Path
import subprocess
import json

root = Path(__file__).resolve().parents[2]
lane = root / "lanes/f-review9"
cases = []
def add(line, expected):
    cases.append((line, expected))

# All thirteen text kinds; the final six have no typed parser in this driver.
types = [("1", "1", "error: DOMAIN"), ("1 mod 4", "error: DOMAIN", "error: DOMAIN"),
         ("(1 ; 1)", "(1 ; 1)", "error: DOMAIN"),
         ("((1) + (0)*i ; 1)", "error: DOMAIN", "error: DOMAIN"),
         ("[1]", "error: DOMAIN", "error: DOMAIN"),
         ("(1 ; 1 * [1])", "(1 ; 1 * [1])", "error: DOMAIN"),
         ("<1 ; [1]>", "error: DOMAIN", "error: DOMAIN")]
for text in ["[p=5: 3 + O(5^4)]", "{}", "(0.5 ; 0) + Q",
             "ffun(D=1, M=1; (0) + (0)*i)", "rfun()", "char(q=1, n=1, s=(0) + (0)*i)"]:
    types.append((text, "error: UNSUPPORTED", "error: UNSUPPORTED"))
for value, root_want, series_want in types:
    add(f"root {value} with 1", root_want)
    for fn in ["exp", "sin", "sinh", "cos", "cosh"]:
        add(f"{fn} {value}", series_want)
for n in ["-1", "1/2", "18446744073709551616", "0"]:
    add(f"root 4 with {n}", "error: DOMAIN")
for s in ["0", "2", "-1", "1/2", "18446744073709551616", "-2147483649", "2147483648"]:
    add(f"root 8 with 3 with {s}", "error: DOMAIN")
for s in ["0", "2", "-1", "2147483647", "-2147483648"]:
    add(f"root 4 with 1 with {s}", "4")
add("root 0 with 3 with -1", "0")
add("root (0 ; 0) with 3 with -1", "(0 ; 0)")
add("root (0 +/- 1 ; 0) with 3 with -1", "error: DOMAIN")
add("root (4 ; 9/4) with 2 with -1", "(-2 ; -3/2)")
for n in ["9223372036854775807", "9223372036854775808", "18446744073709551615"]:
    add(f"root 1 with {n}", "1")
    add(f"root 2 with {n}", "error: DOMAIN")
for line in ["root", "root 4", "root 4 with 2 with 1 with 1", "root 4 with",
             "root 4 with 2 with", "exp", "exp 0 with 1", "root (4 ; 4 with 2",
             "root 4 with 2) with 1", "root 4 with [", "root 4\x00 with 2",
             "root 4 with 2 with 1 # injected with 5", "root 4 with 2 with 1 with " + "1"*10000]:
    add(line, "error: PARSE")
for fn in ["exp", "sin", "sinh", "cos", "cosh"]:
    add(f"{fn} (0 ; 0)", "(1 ; 1)" if fn in ["exp", "cos", "cosh"] else "(0 ; 0)")
    add(f"{fn} (0 ; 0 mod 4)", "error: NOT_DETERMINED")
    add(f"{fn} (0 ; 4 mod 8)", "error: NOT_DETERMINED")

# Three old commands per fixed arity, all tested with one too few and one too many.
old = {1: ["show", "neg", "inv"], 2: ["add", "mul", "cap"],
       3: ["roots_at", "powunit_at", "recover"], 4: ["root_at", "powrat_at", "root_at"]}
for arity, commands in old.items():
    for command in commands:
        for n in [arity - 1, arity + 1]:
            add(command + (" " + " with ".join(["1"] * n) if n else ""), "error: PARSE")

payload = "\n".join(line for line, _ in cases) + "\n"
(lane / "driver-attacks.cmd").write_bytes(payload.encode())
(lane / "driver-attacks.expected").write_text("\n".join(w for _, w in cases) + "\n")
r = subprocess.run(["timeout", "180", str(lane / "adf")], input=payload.encode(), capture_output=True)
(lane / "driver-attacks.out").write_bytes(r.stdout)
got = r.stdout.decode().splitlines()
errors = []
for k, (line, expected) in enumerate(cases):
    actual = got[k] if k < len(got) else "<missing>"
    if actual != expected:
        errors.append(dict(case=k + 1, command=line, expected=expected, actual=actual))
if len(got) != len(cases):
    errors.append(dict(expected_lines=len(cases), actual_lines=len(got)))
print(json.dumps(dict(custom_cases=len(cases), exit=r.returncode, mismatches=errors), indent=2))

goldens = ["gfunc-root", "gfunc-series", "10_line_language", "12_status_order", "root-status", "pow-status"]
for name in goldens:
    cmd = (root / "tests/driver" / f"{name}.cmd").read_bytes()
    expected = (root / "tests/driver" / f"{name}.out").read_bytes()
    r = subprocess.run(["timeout", "180", str(lane / "adf")], input=cmd, capture_output=True)
    (lane / f"driver-{name}.out").write_bytes(r.stdout)
    print(json.dumps(dict(golden=name, lines=len(expected.splitlines()), exit=r.returncode,
                          equal=r.stdout == expected)))
