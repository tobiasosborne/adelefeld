# Independent literal rows of docs/api-j.md section 2; no header or live-query generation.
using Libdl
length(ARGS) == 1 || error("usage: abi_probe.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(abspath(ARGS[1]))
sym(s) = Libdl.dlsym(LIB, s)
const rows = [
    (:rat,16,8), (:fball,48,8), (:adele,96,8), (:cadele,144,8),
    (:scaled,40,8), (:place,8,8), (:text_limits,32,8), (:ctx_desc,24,8),
    (:text_kind,4,4), (:lball,48,8), (:sball,120,8), (:ucoset,16,8),
    (:idele,80,8), (:idclass,64,8), (:qclass,24,8), (:char,120,8),
    (:ffun,24,8), (:rfun,16,8), (:rterm,312,8),
]
@assert sizeof(Ptr{Cvoid}) == sizeof(Csize_t) == sizeof(Int64) == sizeof(UInt64) == 8
@assert sizeof(Cint) == 4
@assert ccall(sym(:adf_version_check), Cint, ()) == 0
for (suffix, size, align) in rows
    actual_size = ccall(sym(Symbol("adf_sizeof_", suffix)), Csize_t, ())
    actual_align = ccall(sym(Symbol("adf_alignof_", suffix)), Csize_t, ())
    @assert actual_size == size && actual_align == align
    println(suffix, " size=", actual_size, " align=", actual_align)
end
const names = ["OK", "NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT", "NOT_UNIQUE",
               "NO_SOLUTION", "NOT_UNIT", "DOMAIN", "UNSUPPORTED", "PARSE", "LIMIT"]
status_name(i) = unsafe_string(ccall(sym(:adf_status_str), Ptr{UInt8}, (Cint,), i))
for (i, name) in enumerate(names)
    @assert status_name(i - 1) == name
end
@assert status_name(-1) == status_name(length(names)) == "UNKNOWN"
count_symbol = Libdl.dlsym(LIB, :adf_status_count; throw_error=false)
@assert count_symbol === nothing
println("rows=", length(rows), " size_align_comparisons=", 2length(rows),
        " status_names=", length(names), " invalid_statuses=2 version_checks=1")
println("effective_status_count=11 exported_macro_accessor=absent")
