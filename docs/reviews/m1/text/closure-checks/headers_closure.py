"""Current export and header check after the old failure assertion became obsolete."""
import json
import re
import subprocess as sp
from pathlib import Path

root = Path(__file__).resolve().parents[5]
scratch = root / "build-closure"
aux = scratch / "declarations.aux"
unit = scratch / "declarations.c"
unit.write_text("#include <adelefeld.h>\n")
sp.run(["cc", "-Iinclude", "-std=c11", "-aux-info", str(aux), "-c", str(unit),
        "-o", str(scratch / "declarations.o")], cwd=root, check=True)
names = set()
for line in aux.read_text().splitlines():
    if line.startswith("/* include/adelefeld/"):
        match = re.search(r"\b(adf_\w+)\s*\(", line)
        if match:
            names.add(match[1])
nm = sp.check_output(["nm", "-D", "--defined-only", str(scratch / "bridge.so")], text=True)
exported = {line.split()[-1] for line in nm.splitlines()}
missing = sorted(names - exported)
assert len(names) == 193 and not missing
required = ("adf_rat_dump_str", "adf_scaled_get_str", "adf_cadele_dump_inspect",
            "adf_modctx_new_from_dump")
assert all(name in exported for name in required)
flint = sp.check_output(["nm", "-D", "--defined-only", "/usr/lib/x86_64-linux-gnu/libflint.so"],
                        text=True)
flint_names = {line.split()[-1] for line in flint.splitlines()}
assert "arb_add_fmpq" not in flint_names and "arb_mul_fmpq" not in flint_names
header = (root / "include/adelefeld/adele.h").read_text()
assert "has no arb_add_fmpq, arb_mul_fmpq" in header
print(json.dumps({"declared": len(names), "exported": len(names & exported), "missing": len(missing),
                  "required_present": len(required), "false_flint_symbols": 0}))
