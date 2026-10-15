# Run only the J-b.1 section of test/runtests.jl of the package in the active project (a fault copy).
# A crashing fault aborts the process, and Julia's libuv stdout and stderr lose what is still queued (the
# failures printed before the crash). Raw descriptor writers are synchronous, so those lines survive.
Core.eval(Base, :(stdout = $(Base.Filesystem.File(RawFD(1)))))
Core.eval(Base, :(stderr = $(Base.Filesystem.File(RawFD(2)))))
using Test
using Adelefeld
using Adelefeld: OK, DOMAIN, NOT_UNIT, PARSE, LIMIT
const PKG = dirname(Base.active_project())
src = read(joinpath(PKG, "test", "runtests.jl"), String)
i = findfirst("# ---- Step J-b.1:", src)
i === nothing && error("no J-b.1 section")
include_string(Main, src[first(i):end], "runtests.jl(J-b.1)")
