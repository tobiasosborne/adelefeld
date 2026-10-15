"""Run one planted J-b.1 fault in a lane scratch copy of the package. Production files are never changed."""
from pathlib import Path
import os, re, shutil, subprocess, sys

# A crashing fault must not leave a large core on the shared disk: the machine's core pattern pipes to
# systemd-coredump with a fixed size limit (so RLIMIT_CORE is ignored). coredump_filter 0 keeps no memory
# mapping in a core; it is inherited across fork and execve, so the Julia children get it too.
with open("/proc/self/coredump_filter", "w") as fh:
    fh.write("0")

root = Path.cwd()
lane = root / "lanes/j-slice5"
fault = sys.argv[1]
scratch = lane / ("scratch-" + fault) / "Adelefeld"
source = root / "julia/Adelefeld"

def change(file, before, after, count=1):
    path = scratch / file
    text = path.read_text()
    assert text.count(before) == count, (file, before, text.count(before))
    path.write_text(text.replace(before, after))

FAULTS = {
    # context proof step 2: roots stored but no token taken
    "F1": [("src/context.jl", "            _acquire!(m.state)\n            taken += 1",
            "            taken += 1")],
    # step 4: a value never drops its tokens
    "F2": [("src/fball.jl", "        _drop_roots!(x)    # after the whole clear (context proof step 4)\n", "")],
    # step 4/5: the ModCtx finalizer frees the C context directly, ignoring the value tokens
    "F3": [("src/context.jl", "        claimed || return nothing\n        _drop!(c.state)",
            "        claimed || return nothing\n        @atomic c.state.count = 0\n"
            "        ccall(p_adf_modctx_free[], Cvoid, (P,), c.state.ptr)\n"
            "        Threads.atomic_sub!(CTX_LIVE, 1); Threads.atomic_add!(CTX_FREES, 1)")],
    # step 5: off by one, the decrement that sees 2 frees
    "F4": [("src/context.jl", "    if old == 1\n", "    if old == 2\n")],
    # one token per DISTINCT state: duplicates pushed
    "F5": [("src/context.jl", "    any(r -> r === m, acc) || push!(acc, m)", "    push!(acc, m)")],
    # lost read on failure: details written on a non-OK status
    "F6": [("src/modctx.jl", "    r = _result(z, raw, :enclose_local)\n    r.status === OK || return r",
            "    r = _result(z, raw, :enclose_local)\n"
            "    r.status === OK || return AdfResult{FBall}(r.status, nothing, nothing, (lost = lost[] != 0,))")],
    # the exact conversion does not lease its explicit context
    "F7": [("src/modctx.jl", "    pc = _cptr(ctx)          # refused before C if finalized; the lease below keeps pc valid\n"
            "    z = _new_fball(x, ctx)", "    pc = _cptr(ctx)\n    z = _new_fball(x)")],
    # the pointer slot read on failure
    "F8": [("src/modctx.jl", "    st === OK || return AdfResult{ModCtx}(st, nothing, nothing, details)\n    return",
            "    st === OK || return AdfResult{ModCtx}(st, out[] == C_NULL ? nothing : _own_modctx(out[]), "
            "nothing, details)\n    return")],
    # require exactly one family: the first one given is taken
    "F9": [("src/modctx.jl", "    length(given) == 1 || throw", "    length(given) >= 1 || throw")],
    # copy of a local value without a lease
    "F10": [("src/fball.jl", "    z = _new_fball(x)\n    GC.@preserve z x ccall(p_adf_fball_set[]",
             "    z = _new_fball()\n    GC.@preserve z x ccall(p_adf_fball_set[]")],
    # an adele from a local finite ball without a lease
    "F11": [("src/carrier.jl", "    z = _new_adele(r._carrier, f)", "    z = _new_adele()")],
    # tokens dropped BEFORE the C clear (order of step 4)
    "F12": [("src/fball.jl", "        _drop_roots!(x)    # after the whole clear (context proof step 4)\n", ""),
            ("src/fball.jl", "        claimed || return nothing\n        ccall(p_adf_fball_clear[]",
             "        claimed || return nothing\n        _drop_roots!(x)\n        ccall(p_adf_fball_clear[]")],
    # a finalized context accepted as an argument
    "F13": [("src/context.jl", "    (@atomic c.owner) || throw(InvalidStateException(\"the ModCtx was finalized\", :closed))\n", "")],
    # a block outside 0..2^64-1 wrapped into a word
    "F14": [("src/modctx.jl", "    0 <= v <= typemax(UInt64) || throw", "    v = v % UInt64\n    0 <= v <= typemax(UInt64) || throw")],
    # finitepart of an adele with a local finite coordinate without a lease
    "F15": [("src/adele.jl", "            z = _new_fball(x)\n            GC.@preserve z x ccall($getfin[]",
             "            z = _new_fball()\n            GC.@preserve z x ccall($getfin[]")],
    # to_local reports the enclosing variant's status mapping: DOMAIN replaced by the enclosure
    "F16": [("src/modctx.jl", "ccall(p_adf_fball_set_local[], Cint, (P, P, P), _ptr(z), _ptr(x), pc)",
             "ccall(p_adf_fball_set_local_enclose[], Cint, (P, Ptr{Cint}, P, P), _ptr(z), C_NULL, _ptr(x), pc)")],
}

if scratch.parent.exists():
    shutil.rmtree(scratch.parent)
shutil.copytree(source, scratch, ignore=shutil.ignore_patterns("Manifest.toml"))
for f, a, b in FAULTS[fault]:
    change(f, a, b)
env = os.environ.copy()
env["PKGDIR"] = str(scratch)
env["ADELEFELD_LIB"] = str(root / "build/libadelefeld.so")
env["T"] = "300"
log = lane / ("fault-" + fault + ".log")
with log.open("w") as out:
    p = subprocess.run(["sh", str(lane / "run-julia.sh"), str(lane / "jb1_only.jl")], env=env,
                       stdout=out, stderr=subprocess.STDOUT, timeout=320)
text = log.read_text()
m = re.search(r"Adelefeld step J-b\.1\s*\|([^\n]*)", text)
summary = m.group(0) if m else "no completed J-b.1 summary"
crash = "signal" in text.lower() or "segmentation" in text.lower()
failed = text.count("Test Failed")
errored = text.count("Error During Test")
caught = "ASSERTION" if failed + errored > 0 else ("CRASH-ONLY" if crash else "NOT-CAUGHT")
record = (f"{fault} {caught} EXIT={p.returncode} CRASH={int(crash)} FAILED={failed} ERRORED={errored} "
          f"{summary}\n")
with (lane / "fault-summary.log").open("a") as out:
    out.write(record)
print(record, end="")
shutil.rmtree(scratch.parent)
