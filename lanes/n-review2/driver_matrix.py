"""Exercise each documented operand slot with four kinds that formerly escaped dispatch."""
from pathlib import Path
import os
import subprocess

lane = Path("lanes/n-review2")
values = {"unit": "[5 mod 6]", "idele": "(5 ; 5 * [1])", "class": "<1 ; [1]>",
          "complex": "((2) + (3)*i ; 1 mod 6)"}
operations = {
    "show": ["1"], "type": ["1"], "neg": ["1"], "inv": ["1"],
    "norm": ["(1 ; 1 * [1])"], "class": ["(1 ; 1 * [1])"], "idele": ["1"],
    "hull": ["(1 ; 1 * [1])"], "hullsimple": ["(1 ; 1 * [1])"],
    "unitof": ["(1 ; 1)"], "reconstruct": ["(1 ; 1)"], "dump": ["1"], "load": ["1"],
    "realroots": ["-1 1"], "prec": ["64"], "digits": ["20"],
    "add": ["1", "1"], "sub": ["1", "1"], "mul": ["1", "1"], "div": ["1", "1"],
    "equal": ["1", "1"], "contains": ["1", "1"], "overlaps": ["1", "1"],
    "compare": ["1", "1"], "cap": ["1 mod 6", "1"],
    "pow": ["[1]", "2"], "powtight": ["[1]", "2"],
    "valuation": ["(1 ; 1 * [1])", "5"], "abs": ["(1 ; 1 * [1])", "5"],
    "project": ["1", "5"], "exp_at": ["1", "5"], "log_at": ["1", "5"],
    "roots": ["-1 1", "5", "3"], "recover": ["1 mod 6", "2", "2"],
}
allowed = {"show": set(values), "type": set(values), "neg": {"unit", "idele", "complex"},
           "inv": {"unit", "idele", "class"}, "norm": {"idele", "class"},
           "class": {"idele"}, "hull": {"idele"}, "hullsimple": {"idele"},
           "pow": {"unit", "idele", "class"}, "powtight": {"unit", "idele", "class"},
           "valuation": {"idele"}, "abs": {"idele"}, "dump": {"complex"}}
cases = []
for op, args in operations.items():
    for pos in range(len(args)):
        for kind, value in values.items():
            a = args.copy()
            a[pos] = value
            accepts = pos == 0 and kind in allowed.get(op, set())
            if op in ("add", "sub", "mul", "div", "equal", "contains", "overlaps", "compare"):
                accepts = kind == "complex" and (op != "div" or pos == 0)
                accepts |= kind == "idele" and (op == "mul" or (op == "div" and pos == 0))
            cases.append((op + " " + " with ".join(a), accepts))
for op in ("reconstruct",):
    for pos in range(3):
        for value in values.values():
            a = ["1 mod 6", "0", "2"]
            a[pos] = value
            cases.append((op + " " + " with ".join(a), False))
for op in ("project", "exp_at", "log_at"):
    for value in values.values():
        cases.append((f"{op} {value} with real", False))
for op in ("add", "sub", "mul", "div", "equal", "contains", "overlaps", "compare"):
    for ka, va in values.items():
        for kb, vb in values.items():
            accepts = ka == kb == "complex" and op != "div"
            accepts |= ka == kb and op in ("mul", "div") and ka in ("unit", "idele", "class")
            accepts |= ka == kb == "unit" and op in ("equal", "contains", "overlaps")
            cases.append((f"{op} {va} with {vb}", accepts))
(lane / "driver-matrix.cmd").write_text("\n".join(c[0] for c in cases) + "\n")
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
r = subprocess.run(["timeout", "45", str(lane / "adf-san"), str(lane / "driver-matrix.cmd")],
                   text=True, capture_output=True, env=env, timeout=50)
(lane / "driver-matrix.out").write_text(r.stdout)
(lane / "driver-matrix.err").write_text(r.stderr)
lines = r.stdout.splitlines()
bad = 0
rejects = {"error: UNSUPPORTED", "error: DOMAIN", "error: PARSE"}
for (cmd, accepts), answer in zip(cases, lines):
    if not accepts and answer not in rejects:
        print(f"UNEXPECTED {cmd!r} -> {answer!r}")
        bad += 1
bad += len(lines) != len(cases) or r.returncode != 1 or bool(r.stderr)
print(f"cases={len(cases)} lines={len(lines)} allowed_value_cases={sum(c[1] for c in cases)} "
      f"stderr_bytes={len(r.stderr)} driver_exit={r.returncode} failures={bad}")
raise SystemExit(bool(bad))
