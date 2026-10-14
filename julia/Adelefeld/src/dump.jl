# Global lossless dumps, J-a.4; docs/api-j.md:267-274; dump.h:21-52,105-162.
# The only context occurrences are local finite balls and scaled values (dump.h:38-47).
# Global Rat/FBall/Adele/CAdele need none. NULL is passed to the one-context convenience loader.
# A local dump gets DOMAIN for its NULL binding (dump.h:41-42); no contexts are constructed here.

"""
    try_dump_text(x::Union{Rat,FBall,Adele,CAdele}) -> AdfResult{String}
    dump_text(x) -> String

Copy exactly the byte length of `adf_x_dump_str`, with `adf_str_free` in a finally (dump.h:49-52).
This is lossless stored data; it has no digits or precision. The statusless dumper synthesizes OK.
NULL or zero length gives LIMIT as required by the Julia design (api-j.md:186-188).
The C dumper never fails (dump.h:51); the driver's NULL->LIMIT guard is on value printing
(tools/adf/adf.c:632-633), not dumping. Zero length is a defensive Julia refusal.
Cost: C output size plus O(bytes) for the Julia copy.
"""
function try_dump_text end

"""
    try_load_dump(T, s; limits=nothing) -> AdfResult{T}
    load_dump(T, s; kwargs...) -> T

Strict typed C dump loader. Context is NULL; no bindings or working precision. The whole input is
validated before FLINT loading. OK writes a new owner; PARSE/LIMIT/UNSUPPORTED/DOMAIN clear the temporary
and retain the exact status (dump.h:21-47). No auxiliary output is written on failure.
`load_dump(T, dump_text(x))` is identical to x when the load limits and C caps admit the dump
(dump.h:49-52; api-4.md:104-108, N-D25). These four global types have no D1 function-array caps.
Cost: C loader plus the Julia byte copy and fresh owning allocation.
"""
function try_load_dump end

for (T, prefix) in ((:Rat, :rat), (:FBall, :fball), (:Adele, :adele), (:CAdele, :cadele))
    dumper = Symbol(:p_adf_, prefix, :_dump_str)
    loader = Symbol(:p_adf_, prefix, :_load_str)
    newfun = Symbol(:_new_, prefix)
    @eval begin
        function try_dump_text(x::$T)
            _check_live(x)
            len = Ref{Csize_t}(0)
            GC.@preserve x len begin
                s = ccall($dumper[], Ptr{UInt8}, (Ptr{Csize_t}, P),
                          Base.unsafe_convert(Ptr{Csize_t}, len), _ptr(x))
                try
                    (s == C_NULL || len[] == 0) && return AdfResult{String}(LIMIT, nothing)
                    unsafe_load(s + len[]) == 0x00 || error("dump printer: no NUL at s[len]")
                    return AdfResult{String}(OK, unsafe_string(s, len[]))
                finally
                    ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)
                end
            end
        end

        function try_load_dump(::Type{$T}, s::AbstractString; limits::Union{TextLimits,Nothing}=nothing)
            str = String(s)
            lim = Ref(limits === nothing ? TextLimits() : limits)
            z = $newfun()
            raw = GC.@preserve z str lim ccall($loader[], Cint,
                (P, Ptr{UInt8}, Csize_t, P, Ptr{TextLimits}), _ptr(z), pointer(str), ncodeunits(str),
                C_NULL, Base.unsafe_convert(Ptr{TextLimits}, lim))
            return _result(z, raw, :load_dump)
        end
    end
end

dump_text(x::AdfValue) = _unwrap(try_dump_text(x), :dump_text)
load_dump(::Type{T}, s::AbstractString; kwargs...) where {T<:AdfValue} =
    _unwrap(try_load_dump(T, s; kwargs...), :load_dump)
