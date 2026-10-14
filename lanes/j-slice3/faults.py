#!/usr/bin/env python3
"""Plant one fault in a scratch package, run the bounded gate, remove the scratch package.
Sources in julia/Adelefeld are never mutated. Run one id per invocation under timeout 180.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

A = "src/adele.jl"
B = "src/carrier.jl"
L = "src/load.jl"
FAULTS = [
    ("F1 carrier frame removed by wrong width", B, "s[2:end-5]", "s[2:end-4]"),
    ("F2 ring operation precision replaced by 64", A,
     "_ptr(z), _ptr(x), _ptr(y), p)", "_ptr(z), _ptr(x), _ptr(y), Int64(64))"),
    ("F3 set_adele embedding skipped", A,
     "GC.@preserve z x ccall(p_adf_cadele_set_adele[], Cvoid, (P, P), _ptr(z), _ptr(x))",
     "nothing"),
    ("F4 carrier finite-zero assertion omitted", B,
     '@assert string(c) == "0" && string(n) == "0" "numerical carrier finite coordinate must be exact 0"',
     "nothing"),
    ("F5 numerical arb address read outside preserve", B,
     "    raw = GC.@preserve z r f ccall(p_adf_adele_set_arb_fball[], Cint, (P, P, P),\n"
     "                                  _ptr(z), _ptr(r._carrier), _ptr(f))",
     "    pr = _ptr(r._carrier)\n"
     "    raw = GC.@preserve z r f ccall(p_adf_adele_set_arb_fball[], Cint, (P, P, P),\n"
     "                                  _ptr(z), pr, _ptr(f))"),
    ("F6 div_rat status ignored", A, "return _result(z, raw, :div)", "return AdfResult{$T}(OK, z)"),
    ("F7 adele layout row not gated", L,
     '    (suffix = :adele, size = 96, align = 8, source = "docs/api-m1.md:38,45"),\n', ""),
    ("F8 real getter called at a finite place", B,
     "    GC.@preserve z x ccall(p_adf_adele_get_real[], Cvoid, (P, P), _ptr(z), _ptr(x))",
     "    v = Ref{UInt64}(0)\n"
     "    make = Libdl.dlsym(LIBHANDLE[], :adf_place_prime)\n"
     "    at = Libdl.dlsym(LIBHANDLE[], :adf_adele_get_arb_at)\n"
     "    GC.@preserve z x v begin\n"
     "        ccall(make, Cint, (Ref{UInt64}, UInt64), v, UInt64(2))\n"
     "        ccall(at, Cint, (P, P, UInt64), _ptr(z), _ptr(x), v[])\n"
     "    end"),
    ("F9 all numerical binary operations routed to add", B,
     'r = $tr(x._carrier, y._carrier; prec)', 'r = try_add(x._carrier, y._carrier; prec)'),
    ("F10 two-ball complex constructor loses imaginary part", B,
     "_ptr(z), _ptr(re._carrier), _ptr(im._carrier))",
     "_ptr(z), _ptr(re._carrier), _ptr(re._carrier))"),
    ("F11 Adele/CAdele limit default evaluated before prec", A,
     "limits::Union{TextLimits,Nothing}=nothing", "limits::Union{TextLimits,Nothing}=TextLimits()"),
    ("F12 carrier limit default evaluated before prec", B,
     "limits::Union{TextLimits,Nothing}=nothing", "limits::Union{TextLimits,Nothing}=TextLimits()"),
]

root = Path.cwd()
scratch = root / "lanes/j-slice3/scratch"
selected = set(sys.argv[1:])
assert len(selected) == 1, "Pass one id so that each invocation stays below three minutes"
with (root / "lanes/j-slice3/faults.log").open("a") as out:
    for name, rel, old, new in FAULTS:
        ident = name.split()[0]
        if ident not in selected:
            continue
        if scratch.exists():
            shutil.rmtree(scratch)
        pkg = scratch / "Adelefeld"
        shutil.copytree(root / "julia/Adelefeld", pkg, ignore=shutil.ignore_patterns("Manifest.toml"))
        path = pkg / rel
        src = path.read_text()
        assert src.count(old) == 1, (name, src.count(old))
        path.write_text(src.replace(old, new))
        env = dict(os.environ, PKGDIR=str(pkg), ADELEFELD_LIB=str(root / "build/libadelefeld.so"))
        try:
            r = subprocess.run(["timeout", "175", "sh", "lanes/j-slice3/gate.sh"],
                               capture_output=True, text=True, env=env, timeout=178)
            log = r.stdout + r.stderr
            (root / f"lanes/j-slice3/fault-{ident}.log").write_text(log)
            m = re.search(r"Adelefeld step J-a\.3 *\| *(.+)", log)
            if m:
                header = log[:m.start()].splitlines()[-1].split("|")[1].split()
                counts = dict(zip(header, m[1].split()))
                summary = "/".join(counts.get(k, "0") for k in ("Pass", "Fail", "Error", "Total"))
            else:
                summary = "no J-a.3 summary"
            caught = "unverified" if not m else str(int(r.returncode != 0))
            print(f"{name}: exit {r.returncode}; caught={caught}; J-a.3 {summary}", file=out, flush=True)
        finally:
            shutil.rmtree(scratch)
