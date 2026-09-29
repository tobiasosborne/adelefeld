#!/usr/bin/env python3
"""Plant a wrong place only for constant real-root outputs; run the unchanged differential script."""
import os
from pathlib import Path
import subprocess

lane = Path("lanes/r-review1")
text = Path("src/roots.c").read_text()
old = """    L->place = adf_place_inf();
    L->scope = ADF_ROOTLIST_PARTITION;
    L->reduced = reduced;
    L->complete = 1;
    fmpz_poly_swap(L->g, g);"""
new = """    L->place = adf_place_inf();
    if (fmpz_poly_degree(g) == 0)
        adf_place_prime(&L->place, 2);
    L->scope = ADF_ROOTLIST_PARTITION;
    L->reduced = reduced;
    L->complete = 1;
    fmpz_poly_swap(L->g, g);"""
assert text.count(old) == 1
(lane / "mutant_roots.c").write_text(text.replace(old, new))
src = [str(p) for p in sorted(Path("src").glob("*.c")) if p.name != "roots.c"]
subprocess.run(["timeout", "60", "cc", "-shared", "-fPIC", "-Iinclude", "-std=c11", "-O2", "-g",
                *src, str(lane / "mutant_roots.c"), "-o", str(lane / "mutant.so"),
                "-lflint", "-lgmp", "-lm"], check=True)
subprocess.run(["timeout", "45", "python3", "tests/fuzz/diff_roots_real.py", "--seconds", "30",
                "--seed", "1", "--lib", str(lane / "mutant.so")], check=True,
               env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"})
import ctypes
lib = ctypes.CDLL(str(lane / "mutant.so"))
from importlib.util import spec_from_file_location, module_from_spec
spec = spec_from_file_location("diff", "tests/fuzz/diff_roots_real.py")
module = module_from_spec(spec)
spec.loader.exec_module(module)
br = module.Bridge(str(lane / "mutant.so"))
out = br.run([7], 2)
lib.adf_rootlist_place.argtypes = [ctypes.c_void_p]
lib.adf_rootlist_place.restype = ctypes.c_ulong
lib.adf_place_is_archimedean.argtypes = [ctypes.c_ulong]
lib.adf_place_is_archimedean.restype = ctypes.c_int
place = lib.adf_rootlist_place(br.L)
print("planted_fault_status", out["status"], "canonical", out["canonical"],
      "entries", out["ve"], "complete", out["vc"], "place_is_real", lib.adf_place_is_archimedean(place))
