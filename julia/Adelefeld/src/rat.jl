# TextLimits and the Rat handle (docs/api-j.md sections 3 and 4).
# C contract: include/adelefeld/rat.h (life cycle 40-60, constructors 62-88, predicates 90-98,
# arithmetic 100-116, layout 118-120) and include/adelefeld/text.h (parser and printer rules 17-58,
# limits 89-115, adf_rat_set_str and adf_rat_get_str 146-154); common.h:66-70 (adf_str_free).
# Julia 1.12.5 help (lanes/j-slice1/help.log): finalizer "must not cause a task switch" (59-94); GC.@preserve
# covers "Passing a pointer to x to ccall" and "resources of x which would be cleaned up in the finalizer"
# (95-135); InvalidStateException (136-157); @atomicreplace returns "(old, success::Bool)" (158-230);
# ncodeunits (245-276); unsafe_string(p, length) needs no terminator (277-285).

const P = Ptr{Cvoid}

"""
    TextLimits(; max_len, max_exp10, max_prec, max_items)

The limits of a parser (conventions 8.4; include/adelefeld/text.h:81-103): layout `adf_text_limits_t`,
32 bytes, fields at 0, 8, 16, 24 (checked at load time). Fields not given take the defaults that
`adf_text_limits_default` writes (text.h:105-115), never duplicate literals (docs/api-j.md:77-78).
A negative or too large value is an `InexactError`, never a wrapped word.
"""
struct TextLimits
    max_len::Csize_t
    max_exp10::Int64
    max_prec::Int64
    max_items::Int64
end

function TextLimits(; max_len=nothing, max_exp10=nothing, max_prec=nothing, max_items=nothing)
    require_loaded()
    r = Ref(TextLimits(0, 0, 0, 0))
    ccall(p_adf_text_limits_default[], Cvoid, (Ref{TextLimits},), r)
    d = r[]
    return TextLimits(max_len === nothing ? d.max_len : Csize_t(max_len),
                      max_exp10 === nothing ? d.max_exp10 : Int64(max_exp10),
                      max_prec === nothing ? d.max_prec : Int64(max_prec),
                      max_items === nothing ? d.max_items : Int64(max_items))
end

"""
    Rat

The exact global rational `adf_rat` (include/adelefeld/rat.h:1-6; SPEC 4.1): a mutable handle that owns
aligned C heap storage holding one initialized `adf_rat_struct` (decision J4). A `Number` with only the
methods defined here: no ordering, no implicit Float64 or Complex conversion (docs/api-j.md:95-97, 232-233).
"""
struct _Raw end   # private token: no public constructor makes a handle from raw pointers

mutable struct Rat <: Number
    base::Ptr{Cvoid}
    data::Ptr{Cvoid}
    @atomic state::Int
    function Rat(::_Raw)
        require_loaded()
        base, data = aligned_alloc(RAT_SIZE, RAT_ALIGN)
        return new(base, data, STATE_NEW)
    end
end

_rat_storage() = Rat(_Raw())

# A fresh, initialized Rat (value 0, rat.h:42). adf init runs once, then the finalizer is registered;
# if registration fails the value is cleared and freed at once (docs/api-j.md:103).
function _new_rat()
    x = _rat_storage()
    GC.@preserve x ccall(p_adf_rat_init[], Cvoid, (P,), x.data)
    @atomic x.state = STATE_LIVE
    Threads.atomic_add!(LIVE_HANDLES, 1)
    try
        finalizer(_release!, x)
    catch
        _release!(x)
        rethrow()
    end
    return x
end

# The private release (docs/api-j.md:107-113): claims LIVE once, calls the cached void clear (rat.h:45-46,
# no status), frees BASE (never the adjusted pointer). Repeated close or finalize is a no-op. No I/O, task
# switch, symbol resolution or status translation; an unexpected exception is counted and leaks.
function _release!(x::Rat)
    try
        (_, claimed) = @atomicreplace x.state STATE_LIVE => STATE_CLOSED
        claimed || return nothing
        ccall(p_adf_rat_clear[], Cvoid, (P,), x.data)
        Libc.free(x.base)
        x.base = C_NULL
        x.data = C_NULL
        Threads.atomic_sub!(LIVE_HANDLES, 1)
    catch
        Threads.atomic_add!(FINALIZER_ERRORS, 1)
    end
    return nothing
end

"""
    close(x::Rat)

Clears and frees the C value now. Repeated close is a no-op; a closed handle throws
`InvalidStateException` before any C call. Must not race with a use of `x` (docs/api-j.md:107-109).
"""
Base.close(x::Rat) = _release!(x)

# The address of a live value; a closed handle throws before C (docs/api-j.md:108-109).
function _ptr(x::Rat)
    (@atomic x.state) == STATE_LIVE || throw(InvalidStateException("the Rat handle is closed", :closed))
    return x.data
end

# ---- statuses of C calls that return one ----

# A fresh output z was written by a C function that returned raw. On OK the result owns z; on every other
# status z is cleared at once and the result has no value (docs/api-j.md:167). An unknown raw value
# clears z and throws AdfAbiError.
function _result(z::Rat, raw::Cint, op::Symbol)
    if raw != 0
        _release!(z)
        return AdfResult{Rat}(to_status(raw, op), nothing)
    end
    return AdfResult{Rat}(OK, z)
end

# ---- constructors (docs/api-j.md:195-202) ----

"""
    try_parse(Rat, s; limits=TextLimits()) -> AdfResult{Rat}

Calls `adf_rat_set_str` on the bytes of `s` (`ncodeunits`, not characters). NUL and non-ASCII bytes go to C,
which keeps its order of checks: LIMIT if the byte count exceeds `limits.max_len`, PARSE for a byte or a text
outside the grammar `rat = int ["/" uint]`, LIMIT on literal limits, DOMAIN for a zero denominator
(text.h:17-27, 147-150; conventions 9.2). Cost: the C reader plus a byte copy.
"""
function try_parse(::Type{Rat}, s::AbstractString; limits::TextLimits=TextLimits())
    str = String(s)
    x = _new_rat()
    lim = Ref(limits)
    raw = GC.@preserve x str lim ccall(p_adf_rat_set_str[], Cint, (P, Ptr{UInt8}, Csize_t, Ptr{TextLimits}),
                                       _ptr(x), pointer(str), ncodeunits(str),
                                       Base.unsafe_convert(Ptr{TextLimits}, lim))
    return _result(x, raw, :parse)
end

Base.parse(::Type{Rat}, s::AbstractString; kwargs...) = _unwrap(try_parse(Rat, s; kwargs...), :parse)

"""
    try_rat(n::Integer, d::Integer=1; limits=TextLimits()) -> AdfResult{Rat}
    try_rat(q::Rational; limits=TextLimits())

`n` alone, when it fits Int64, goes through `adf_rat_set_si` (statusless, OK is synthesized). Otherwise the
pair is normalized to a positive denominator by exact BigInt sign changes and read by `adf_rat_set_str`
from the decimal text "n/d" (docs/api-j.md:195-197); `d = 0` gives DOMAIN; a text longer than
`limits.max_len` gives LIMIT (pass larger limits for very large integers).
"""
function try_rat(n::Integer, d::Integer=1; limits::TextLimits=TextLimits())
    if d == 1 && typemin(Int64) <= n <= typemax(Int64)
        x = _new_rat()
        GC.@preserve x ccall(p_adf_rat_set_si[], Cvoid, (P, Int64), _ptr(x), Int64(n))
        return AdfResult{Rat}(OK, x)
    end
    N = BigInt(n)
    D = BigInt(d)
    if D < 0
        N = -N
        D = -D
    end
    return try_parse(Rat, string(N, "/", D); limits=limits)
end

try_rat(q::Rational; kwargs...) = try_rat(numerator(q), denominator(q); kwargs...)

Rat(n::Integer, d::Integer=1; kwargs...) = _unwrap(try_rat(n, d; kwargs...), :rat)
Rat(q::Rational; kwargs...) = _unwrap(try_rat(q; kwargs...), :rat)

# ---- text (docs/api-j.md:186-193; text.h:29-34, 152-154) ----

"""
    try_value_text(x::Rat) -> AdfResult{String}

`adf_rat_get_str`: "n" or "n/d" in lowest terms (conventions 9.4). Exactly `len` bytes are copied; the
terminator at `s[len]` is checked; the C string is always released with `adf_str_free`. A NULL pointer or
`len = 0` becomes LIMIT, as the driver does (the rat printer never returns NULL, text.h:49).
"""
function try_value_text(x::Rat)
    len = Ref{Csize_t}(0)
    GC.@preserve x len begin
        s = ccall(p_adf_rat_get_str[], Ptr{UInt8}, (Ptr{Csize_t}, P),
                  Base.unsafe_convert(Ptr{Csize_t}, len), _ptr(x))
        try
            (s == C_NULL || len[] == 0) && return AdfResult{String}(LIMIT, nothing)
            unsafe_load(s + len[]) == 0x00 || error("adf_rat_get_str: no NUL at s[len] (text.h:30-31)")
            return AdfResult{String}(OK, unsafe_string(s, len[]))
        finally
            ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)
        end
    end
end

value_text(x::Rat) = _unwrap(try_value_text(x), :value_text)

Base.show(io::IO, x::Rat) = print(io, value_text(x))

# ---- copy, identity, equality (docs/api-j.md:224-229, 235) ----

"""
    copy(x::Rat)

A fresh owner set by `adf_rat_set` (rat.h:48); it has its own storage and its own clear.
"""
function Base.copy(x::Rat)
    px = _ptr(x)
    z = _new_rat()
    GC.@preserve z x ccall(p_adf_rat_set[], Cvoid, (P, P), _ptr(z), px)
    return z
end

"""
    identical(x::Rat, y::Rat)

`adf_rat_identical` (rat.h:58-60): the same numerator and denominator; for this exact type the same as `==`.
"""
identical(x::Rat, y::Rat) = GC.@preserve x y ccall(p_adf_rat_identical[], Cint, (P, P), _ptr(x), _ptr(y)) != 0

Base.:(==)(x::Rat, y::Rat) = GC.@preserve x y ccall(p_adf_rat_equal[], Cint, (P, P), _ptr(x), _ptr(y)) != 0

# Value-based hash, consistent with == against Rat, Integer and Rational: the hash of the same
# Rational{BigInt}, which Julia makes equal to the hash of an equal Integer. The value is read from the
# C text (no reinterpretation of FLINT storage).
function _rational(x::Rat)
    s = value_text(x)
    i = findfirst('/', s)
    i === nothing && return parse(BigInt, s) // big(1)
    return parse(BigInt, s[1:prevind(s, i)]) // parse(BigInt, s[nextind(s, i):end])
end

Base.hash(x::Rat, h::UInt) = hash(_rational(x), h)

# ---- arithmetic (docs/api-j.md:208-221; rat.h:100-116) ----

# The exact ring operations are void in C; OK is synthesized (docs/api-j.md:156). The inputs are checked
# live before the output is allocated, so a closed input allocates nothing.
for (name, cfun) in ((:try_add, :p_adf_rat_add), (:try_sub, :p_adf_rat_sub), (:try_mul, :p_adf_rat_mul))
    @eval function $name(x::Rat, y::Rat)
        px = _ptr(x)
        py = _ptr(y)
        z = _new_rat()
        GC.@preserve z x y ccall($cfun[], Cvoid, (P, P, P), _ptr(z), px, py)
        return AdfResult{Rat}(OK, z)
    end
end

function try_neg(x::Rat)
    px = _ptr(x)
    z = _new_rat()
    GC.@preserve z x ccall(p_adf_rat_neg[], Cvoid, (P, P), _ptr(z), px)
    return AdfResult{Rat}(OK, z)
end

"""
    try_div(x::Rat, q::Rat) -> AdfResult{Rat}

`adf_rat_div` (rat.h:109-113): OK, or NOT_UNIT when `q = 0`; the fresh output is cleared on failure.
"""
function try_div(x::Rat, q::Rat)
    px = _ptr(x)
    pq = _ptr(q)
    z = _new_rat()
    raw = GC.@preserve z x q ccall(p_adf_rat_div[], Cint, (P, P, P), _ptr(z), px, pq)
    return _result(z, raw, :div)
end

"""
    try_inv(q::Rat) -> AdfResult{Rat}

`adf_rat_inv` (rat.h:115-116): OK, or NOT_UNIT when `q = 0`.
"""
function try_inv(q::Rat)
    pq = _ptr(q)
    z = _new_rat()
    raw = GC.@preserve z q ccall(p_adf_rat_inv[], Cint, (P, P), _ptr(z), pq)
    return _result(z, raw, :inv)
end

# Integer and Rational scalars by explicit Rat conversion (docs/api-j.md:232); a failing conversion
# (1//0 is DOMAIN, a too long text LIMIT) is returned as that result.
const RatScalar = Union{Integer,Rational}

for name in (:try_add, :try_sub, :try_mul, :try_div)
    @eval function $name(x::Rat, y::RatScalar)
        r = try_rat(y)
        r.status === OK || return r
        return $name(x, r.value)
    end
    @eval function $name(x::RatScalar, y::Rat)
        r = try_rat(x)
        r.status === OK || return r
        return $name(r.value, y)
    end
end

# The throwing forms (decision J5) and the Base operators.
add(x, y) = _unwrap(try_add(x, y), :add)
sub(x, y) = _unwrap(try_sub(x, y), :sub)
mul(x, y) = _unwrap(try_mul(x, y), :mul)
neg(x) = _unwrap(try_neg(x), :neg)
divexact(x, q) = _unwrap(try_div(x, q), :div)
Base.inv(q::Rat) = _unwrap(try_inv(q), :inv)

Base.:-(x::Rat) = neg(x)
for (op, f) in ((:+, :add), (:-, :sub), (:*, :mul), (:/, :divexact))
    @eval Base.$op(x::Rat, y::Rat) = $f(x, y)
    @eval Base.$op(x::Rat, y::RatScalar) = $f(x, y)
    @eval Base.$op(x::RatScalar, y::Rat) = $f(x, y)
end
# Base defines *(::Bool, ::T) for every T <: Number (bool.jl); these two methods resolve the ambiguity
# with the Integer methods above and keep the Rat meaning.
Base.:*(x::Bool, y::Rat) = mul(x, y)
Base.:*(x::Rat, y::Bool) = mul(x, y)

# == with an Integer or Rational scalar: the exact value comparison. A Julia Rational with zero
# denominator (an infinity) is not a rational number, so it equals no Rat.
function _equal_scalar(x::Rat, y::RatScalar)
    y isa Rational && iszero(denominator(y)) && return false
    return x == Rat(y)
end
Base.:(==)(x::Rat, y::RatScalar) = _equal_scalar(x, y)
Base.:(==)(x::RatScalar, y::Rat) = _equal_scalar(y, x)
