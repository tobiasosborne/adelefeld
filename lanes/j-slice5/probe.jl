# Probe: Julia atomic modify semantics and raw C statuses of modctx.h constructors (no package API).
mutable struct S; @atomic count::Int; end
s = S(1)
r = @atomic s.count -= 1
println("ATOMIC_SUBEQ_RETURNS=", r, " FIELD=", s.count)
s2 = S(5); p = modifyfield!(s2, :count, -, 1, :sequentially_consistent)
println("MODIFYFIELD_RETURNS=", p, " TYPE=", typeof(p))
using Adelefeld
const H = Adelefeld.LIBHANDLE[]
f(s) = Libdl.dlsym(H, s)
using Libdl
function blocks_status(q::Vector{UInt64})
    out = Ref{Ptr{Cvoid}}(C_NULL)
    st = ccall(f(:adf_modctx_new_blocks), Cint, (Ref{Ptr{Cvoid}}, Ptr{UInt64}, Int64), out, q, length(q))
    out[] != C_NULL && ccall(f(:adf_modctx_free), Cvoid, (Ptr{Cvoid},), out[])
    return st
end
println("BLOCKS [8,9]=", blocks_status(UInt64[8, 9]), " [1]=", blocks_status(UInt64[1]),
        " []=", blocks_status(UInt64[]), " [6,4]=", blocks_status(UInt64[6, 4]))
q = UInt64.(collect(2:65538)); q[1] = 1   # 65537 blocks, the first is 1 (DOMAIN) and k > 65536
println("BLOCKS k=65537 first=1: ", blocks_status(q))
