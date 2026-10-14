"""Run one J-a.4 mutation in a lane scratch package. Production files are never overwritten."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys

root = Path.cwd()
lane = root / "lanes/j-slice4"
fault = sys.argv[1]
scratch = lane / ("scratch-" + fault)
source = root / "julia/Adelefeld"


def change(file, before, after, count=1):
    path = scratch / file
    text = path.read_text()
    assert text.count(before) >= count, (file, before)
    path.write_text(text.replace(before, after, count))


assert not scratch.exists(), scratch
shutil.copytree(source, scratch, ignore=shutil.ignore_patterns("Manifest.toml"))
try:
    if fault == "F1":
        change("src/text.jl", "    st === OK || return AdfResult{AdfValue}(st, nothing)\n"
               "    k = to_text_kind(kind[])",
               "    k = to_text_kind(kind[])\n    st === OK || return AdfResult{AdfValue}(st, nothing)")
    elif fault == "F2":
        change("src/text.jl", "r = try_parse(FBall, str; limits=limvalue)",
               "r = try_parse(Rat, str; limits=limvalue)")
    elif fault == "F3":
        change("src/dump.jl", "ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)", "nothing # skipped free")
    elif fault == "F4":
        change("src/dump.jl", "C_NULL, Base.unsafe_convert(Ptr{TextLimits}, lim)", "C_NULL, C_NULL")
    elif fault == "F5":
        # Rat's C loader ignores ctx, so this cannot dereference a forged context in the real core.
        # The counted fixture refuses it without a dereference and proves the caller passed non-NULL.
        change("src/dump.jl", "    newfun = Symbol(:_new_, prefix)",
               "    newfun = Symbol(:_new_, prefix)\n    ctxarg = T === :Rat ? Ptr{Cvoid}(1) : C_NULL")
        change("src/dump.jl", "C_NULL, Base.unsafe_convert(Ptr{TextLimits}, lim)",
               "$ctxarg, Base.unsafe_convert(Ptr{TextLimits}, lim)")
    elif fault == "F6":
        change("src/recon.jl", "return _result(q, raw, :reconstruct)",
               "return _result(q, raw == 4 ? Cint(0) : raw, :reconstruct)", count=2)
    elif fault == "F7":
        # Safe dummy digits on invalid input keep C within its precondition. The moved guard
        # still violates the binding's before-C promise, measured by the counted calls.
        change("src/adele.jl", "            d = _checked_digits(digits)",
               "            d = digits isa Integer && 1 <= digits <= 1000000 ? Int64(digits) : Int64(20)")
        change("src/adele.jl", "                try\n                    (s == C_NULL || len[] == 0)",
               "                try\n                    _checked_digits(digits)\n"
               "                    (s == C_NULL || len[] == 0)")
    elif fault == "F8":
        change("test/runtests.jl", "dump_identity_oracle(x, y) = identical(x, y)",
               "dump_identity_oracle(x, y) = value_text(x) == value_text(y)")
    elif fault == "F9":
        change("src/load.jl", "    for r in LAYOUT_ROWS\n", "    for r in LAYOUT_ROWS\n"
               "        r.suffix === :text_kind && continue\n")
    elif fault == "F10":
        change("src/text.jl", "pointer(str), ncodeunits(str)", "pointer(str), length(str)")
    else:
        raise ValueError(fault)
    env = os.environ.copy()
    env["PKGDIR"] = str(scratch)
    env["ADELEFELD_LIB"] = str(root / "build/libadelefeld.so")
    log = lane / ("fault-" + fault + ".log")
    with log.open("w") as out:
        p = subprocess.run(["timeout", "175", "sh", str(lane / "gate.sh")], env=env,
                           stdout=out, stderr=subprocess.STDOUT, timeout=178)
    text = log.read_text()
    lines = [line for line in text.splitlines() if re.match(r"Adelefeld step J-a\.[1-4]\s*\|", line)]
    verified = bool(re.search(r"Adelefeld step J-a\.4\s*\|", text))
    record = f"{fault} EXIT={p.returncode} VERIFIED={int(verified)}\n" + "\n".join(lines) + "\n"
    with (lane / "fault-summary.log").open("a") as out:
        out.write(record)
    print(record, end="")
    if not verified:
        print("No completed J-a.4 summary. This is unverified, not a caught fault.")
    elif p.returncode != 0:
        print("Caught by completed assertions.")
    else:
        print("Survived.")
finally:
    shutil.rmtree(scratch)
