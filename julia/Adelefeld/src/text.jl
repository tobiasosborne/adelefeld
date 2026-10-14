# Untyped value text, J-a.4; docs/api-j.md:181-193.
# Classification is C syntax only (text.h:137-143), followed by the supported typed reader.
# No Julia grammar or coercion. All value readers return global values (text.h:27-28).

const AdfValue = Union{Rat,FBall,Adele,CAdele}

"""
    try_readvalue(s; prec=64, limits=nothing) -> AdfResult
    readvalue(s; kwargs...)

Classify with `adf_text_classify` (text.h:137-143). LIMIT on excess bytes precedes PARSE for forbidden
bytes or syntax. Read the kind only on OK. A supported kind calls its typed C parser; later stages and
statuses are those of that parser. Other kinds give UNSUPPORTED with `details.kind`.
`prec` must fit Int64 before C defaults are queried (api-ja.md finding 32); below 2 is passed unchanged.
Cost: classification O(bytes), the typed reader, and the Julia byte copy.
"""
function try_readvalue(s::AbstractString; prec=64, limits::Union{TextLimits,Nothing}=nothing)
    p = _checked_prec(prec)
    str = String(s)
    limvalue = limits === nothing ? TextLimits() : limits
    lim = Ref(limvalue)
    kind = Ref{Cint}(typemin(Cint))  # invalid sentinel, never interpreted after a failed call
    raw = GC.@preserve str lim kind ccall(p_adf_text_classify[], Cint,
        (Ptr{Cint}, Ptr{UInt8}, Csize_t, Ptr{TextLimits}),
        Base.unsafe_convert(Ptr{Cint}, kind), pointer(str), ncodeunits(str),
        Base.unsafe_convert(Ptr{TextLimits}, lim))
    st = to_status(raw, :readvalue)
    st === OK || return AdfResult{AdfValue}(st, nothing)
    k = to_text_kind(kind[])
    if k === TEXT_RAT
        r = try_parse(Rat, str; limits=limvalue)
    elseif k === TEXT_FBALL
        r = try_parse(FBall, str; limits=limvalue)
    elseif k === TEXT_ADELE
        r = try_parse(Adele, str; prec=p, limits=limvalue)
    elseif k === TEXT_CADELE
        r = try_parse(CAdele, str; prec=p, limits=limvalue)
    else
        return AdfResult{AdfValue}(UNSUPPORTED, nothing, nothing, (kind=k,))
    end
    return _with_kind(r, k)
end

_with_kind(r::AdfResult{T}, k::TextKind) where {T} = AdfResult{T}(r.status, r.value, r.where, (kind=k,))
readvalue(s::AbstractString; kwargs...) = _unwrap(try_readvalue(s; kwargs...), :readvalue)
