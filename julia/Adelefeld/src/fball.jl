# The FBall handle and the scalar mixed arithmetic (docs/api-j.md sections 3 and 4; step J-a.2 of section 9).
# C contract: include/adelefeld/fball.h (common rules 23-35: aliasing 24-26, an output untouched on a status
# other than OK 29-30; layout 53-64; life cycle 77-106; constructors 118-137; accessors 163-170; tight
# arithmetic 190-232; set predicates 234-256; point comparison 258-265) and include/adelefeld/text.h
# (parser and printer rules 17-58; adf_fball_set_str and adf_fball_get_str 156-165); common.h:66-70
# (adf_str_free). Meaning of a value: the set a + N Zhat (SPEC 4.1); arithmetic: SPEC 4.3.
# Julia 1.12.5 help: lanes/j-slice1/help.log (finalizer, GC.@preserve, @atomicreplace, unsafe_string) and
# lanes/j-slice2/help.log (isequal, hash, in, objectid).

"""
    FBall

The finite ball `adf_fball`, the set `a + N Zhat` of finite adeles (include/adelefeld/fball.h:1-11; SPEC 4.1):
a mutable handle that owns aligned C heap storage holding one initialized `adf_fball_struct` of the checked
layout row 48/8 (decision J4; docs/api-m1.md:36,45). Every value made by this package is global and canonical.

A `Number` with only the methods defined here (docs/api-j.md:95-97). There is no `==` on balls: it throws an
`ArgumentError` naming `equal_set` (equality of the sets) and `compare_points` (three-valued comparison of
unknown points), as docs/api-j.md:225, 234 say. `isequal` and `hash` are identity-based (docs/api-j.md:235).
No ordering, no division by a ball, no implicit Float64 or Complex conversion.
"""
mutable struct FBall <: Number
    base::Ptr{Cvoid}
    data::Ptr{Cvoid}
    @atomic state::Int
    roots::CtxRoots      # contexts this value holds tokens on (context.jl; docs/api-j.md:126-141)
    function FBall(::_Raw)
        require_loaded()
        base, data = aligned_alloc(FBALL_SIZE, FBALL_ALIGN)
        return new(base, data, STATE_NEW, nothing)
    end
end

# A fresh, initialized FBall: the exact 0, global (fball.h:79-81). adf init runs once, then the finalizer is
# registered; if registration fails the value is cleared and freed at once (docs/api-j.md:103).
# srcs: the inputs (and explicit contexts) of the C call that will write it; their context roots are leased
# before that call (context.jl, steps 2-3), since a result may borrow an input's context (fball.h:16-20).
function _new_fball(srcs...)
    x = FBall(_Raw())
    GC.@preserve x ccall(p_adf_fball_init[], Cvoid, (P,), x.data)
    @atomic x.state = STATE_LIVE
    Threads.atomic_add!(LIVE_HANDLES, 1)
    try
        finalizer(_release!, x)
    catch
        _release!(x)
        rethrow()
    end
    isempty(srcs) || _lease!(x, srcs...)
    return x
end

# The private release (docs/api-j.md:107-113), as for Rat: claims LIVE once, calls the cached void clear
# (fball.h:83-84: releases A, H, d and the residue array, never the context; no status), frees BASE.
function _release!(x::FBall)
    try
        (_, claimed) = @atomicreplace x.state STATE_LIVE => STATE_CLOSED
        claimed || return nothing
        ccall(p_adf_fball_clear[], Cvoid, (P,), x.data)
        Libc.free(x.base)
        x.base = C_NULL
        x.data = C_NULL
        Threads.atomic_sub!(LIVE_HANDLES, 1)
        _drop_roots!(x)    # after the whole clear (context proof step 4)
    catch
        Threads.atomic_add!(FINALIZER_ERRORS, 1)
    end
    return nothing
end

"""
    close(x::FBall)

Clears and frees the C value now. Repeated close is a no-op; a closed handle throws
`InvalidStateException` before any C call. Must not race with a use of `x` (docs/api-j.md:107-109).
"""
Base.close(x::FBall) = _release!(x)

function _ptr(x::FBall)
    (@atomic x.state) == STATE_LIVE || throw(InvalidStateException("the FBall handle is closed", :closed))
    return x.data
end

# A fresh output z written by a C function that returned raw: on OK the result owns z; on every other status
# the C function left z untouched (fball.h:29-30) and z is cleared at once (docs/api-j.md:167).
function _result(z::FBall, raw::Cint, op::Symbol)
    if raw != 0
        _release!(z)
        return AdfResult{FBall}(to_status(raw, op), nothing)
    end
    return AdfResult{FBall}(OK, z)
end

# ---- constructors (docs/api-j.md:198, 203-204) ----

"""
    FBall(q::Rat)
    FBall(n::Integer); FBall(q::Rational)

The exact rational `q`, a ball of radius 0, through `adf_fball_set_rat` (fball.h:118-120; statusless, OK is
synthesized). Integer and Rational values are converted explicitly to a `Rat` first, as `Rat` does; a
failing conversion (`1//0` is DOMAIN) throws its `AdfError`.
"""
function FBall(q::Rat)
    pq = _ptr(q)
    z = _new_fball()
    GC.@preserve z q ccall(p_adf_fball_set_rat[], Cvoid, (P, P), _ptr(z), pq)
    return z
end

FBall(q::Union{Integer,Rational}) = FBall(Rat(q))

"""
    try_fball(c::Rat, N::Rat) -> AdfResult{FBall}
    FBall(c::Rat, N::Rat)

The ball `c + N Zhat`, global and canonical (centre reduced into [0, N); N = 0 is the point c), through
`adf_fball_set_center_radius` (fball.h:133-137): OK, or DOMAIN when `N < 0`, the output untouched and
cleared. The sign of `N` is passed to C unchanged. Cost: a gcd and a canonicalisation.
"""
function try_fball(c::Rat, N::Rat)
    pc = _ptr(c)
    pN = _ptr(N)
    z = _new_fball()
    raw = GC.@preserve z c N ccall(p_adf_fball_set_center_radius[], Cint, (P, P, P), _ptr(z), pc, pN)
    return _result(z, raw, :fball)
end

FBall(c::Rat, N::Rat) = _unwrap(try_fball(c, N), :fball)

"""
    try_parse(FBall, s; limits=TextLimits()) -> AdfResult{FBall}

Calls `adf_fball_set_str` on the bytes of `s` (`ncodeunits`, not characters). Both forms of the grammar
`fball_v = "(" "*" ";" fin ")" | rat "mod" urat` (conventions 9.2) are read: `(* ; 3 mod 12)` and `3 mod 12`
(text.h:158-161). Statuses: LIMIT when the byte count exceeds `limits.max_len` (before any byte is read) or
on literal limits, PARSE for a text that is not a sentence (a NUL or non-ASCII byte included), DOMAIN for a
zero denominator (text.h:17-27, 159-160). The result is global and canonical. Cost: the C reader plus a copy.
"""
function try_parse(::Type{FBall}, s::AbstractString; limits::TextLimits=TextLimits())
    str = String(s)
    x = _new_fball()
    lim = Ref(limits)
    raw = GC.@preserve x str lim ccall(p_adf_fball_set_str[], Cint, (P, Ptr{UInt8}, Csize_t, Ptr{TextLimits}),
                                       _ptr(x), pointer(str), ncodeunits(str),
                                       Base.unsafe_convert(Ptr{TextLimits}, lim))
    return _result(x, raw, :parse)
end

Base.parse(::Type{FBall}, s::AbstractString; kwargs...) = _unwrap(try_parse(FBall, s; kwargs...), :parse)

# ---- text (docs/api-j.md:186-193; text.h:32-58, 163-165) ----

"""
    try_value_text(x::FBall) -> AdfResult{String}

`adf_fball_get_str`: the template `(* ; F)` of conventions 9.4, `F` = `q(a)` for a point and `q(a) mod q(N)`
with the centre a in [0, N) otherwise (text.h:163-165). The text is returned with the frame the C printer
writes, unchanged: the design prints `(* ; 2 mod 6)` for the session line (docs/api-j.md:313-314, 418).
Exactly `len` bytes are copied; the terminator at `s[len]` is checked; the C string is always released
with `adf_str_free`, also when the copy or the check throws. A NULL pointer or `len = 0` becomes LIMIT, as
the driver does (the fball printer never returns NULL, text.h:49). No `digits` keyword: the printer has none.
"""
function try_value_text(x::FBall)
    len = Ref{Csize_t}(0)
    GC.@preserve x len begin
        s = ccall(p_adf_fball_get_str[], Ptr{UInt8}, (Ptr{Csize_t}, P),
                  Base.unsafe_convert(Ptr{Csize_t}, len), _ptr(x))
        try
            (s == C_NULL || len[] == 0) && return AdfResult{String}(LIMIT, nothing)
            unsafe_load(s + len[]) == 0x00 || error("adf_fball_get_str: no NUL at s[len] (text.h:33-35)")
            return AdfResult{String}(OK, unsafe_string(s, len[]))
        finally
            ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)
        end
    end
end

value_text(x::FBall) = _unwrap(try_value_text(x), :value_text)

Base.show(io::IO, x::FBall) = print(io, value_text(x))

# ---- copy, identity, canonical predicate (docs/api-j.md:224, 229) ----

"""
    copy(x::FBall)

A fresh owner set by `adf_fball_set` (fball.h:86-88), with its own storage and its own clear.
"""
function Base.copy(x::FBall)
    px = _ptr(x)
    z = _new_fball(x)
    GC.@preserve z x ccall(p_adf_fball_set[], Cvoid, (P, P), _ptr(z), px)
    return z
end

"""
    identical(x::FBall, y::FBall) -> Bool

`adf_fball_identical` (fball.h:102-106): the same backend, context pointer and fields. Representation
identity, not equality of sets (use `equal_set`). For two global canonical values the canonical triple is
unique (conventions Lemma 5.2), so the two agree on every value this step can make.
"""
identical(x::FBall, y::FBall) =
    to_bool(GC.@preserve(x, y, ccall(p_adf_fball_identical[], Cint, (P, P), _ptr(x), _ptr(y))), :identical)

"""
    is_canonical(x::FBall) -> Bool

`adf_fball_is_canonical` (fball.h:93-100): predicate G or L holds. A Bool, never a status.
"""
is_canonical(x::FBall) =
    to_bool(GC.@preserve(x, ccall(p_adf_fball_is_canonical[], Cint, (P,), _ptr(x))), :is_canonical)

# ---- accessors (fball.h:163-170): fresh Rats, never a pointer into the ball (docs/api-j.md:128) ----

"""
    center(x::FBall) -> Rat

The stored centre a = A/d of the canonical triple: in [0, N) for N > 0, the point itself for N = 0
(`adf_fball_get_center`, fball.h:163-166). Any other element of a + N Z is an equally valid centre.
"""
function center(x::FBall)
    px = _ptr(x)
    c = _new_rat()
    GC.@preserve c x ccall(p_adf_fball_get_center[], Cvoid, (P, P), _ptr(c), px)
    return c
end

"""
    radius(x::FBall) -> Rat

The radius N = H/d >= 0 in lowest terms, 0 for an exact value (`adf_fball_get_radius`, fball.h:168-170).
"""
function radius(x::FBall)
    px = _ptr(x)
    N = _new_rat()
    GC.@preserve N x ccall(p_adf_fball_get_radius[], Cvoid, (P, P), _ptr(N), px)
    return N
end

# ---- set predicates and point comparison (fball.h:234-265; docs/api-j.md:222-227) ----

"""
    equal_set(x::FBall, y::FBall) -> Bool

`adf_fball_equal_set` (fball.h:239-242): the two sets are equal. An int 0/1 read as a Bool, never a status.
"""
equal_set(x::FBall, y::FBall) =
    to_bool(GC.@preserve(x, y, ccall(p_adf_fball_equal_set[], Cint, (P, P), _ptr(x), _ptr(y))), :equal_set)

"""
    overlaps(x::FBall, y::FBall) -> Bool

`adf_fball_overlaps` (fball.h:244-245): the two sets meet. Not transitive (SPEC 4.2).
"""
overlaps(x::FBall, y::FBall) =
    to_bool(GC.@preserve(x, y, ccall(p_adf_fball_overlaps[], Cint, (P, P), _ptr(x), _ptr(y))), :overlaps)

"""
    inside(x::FBall, y::FBall) -> Bool

True when the set `x` is inside the set `y`: FIRST INSIDE SECOND (SPEC 4.2; docs/api-j.md:222, 314).
The C function is `adf_fball_contains(x, y)`, whose comment says "1 if the set x is inside the set y"
(fball.h:248-251): despite its verb, its first argument is the smaller set, so the arguments are passed in
the same order, `inside(x, y)` -> `adf_fball_contains(x, y)`. Not `Base.contains` (a text search).
"""
inside(x::FBall, y::FBall) =
    to_bool(GC.@preserve(x, y, ccall(p_adf_fball_contains[], Cint, (P, P), _ptr(x), _ptr(y))), :inside)

"""
    compare_points(x::FBall, y::FBall) -> PointComparison

`adf_fball_compare` (fball.h:258-265): the comparison of two unknown points, one in each ball. EQUAL if both
are exact and equal, DIFFERENT if the sets are disjoint, UNDECIDED otherwise. A `PointComparison`, not a
status; a raw value outside 0..2 is an `AdfAbiError`.
"""
compare_points(x::FBall, y::FBall) =
    to_point_comparison(GC.@preserve(x, y, ccall(p_adf_fball_compare[], Cint, (P, P), _ptr(x), _ptr(y))),
                        :compare_points)

# ---- tight arithmetic (SPEC 4.3; fball.h:190-232; docs/api-j.md:208-219) ----

# The ring operations on two balls are void in C; OK is synthesized (docs/api-j.md:156). The inputs are
# checked live before the output is allocated. Aliasing of the inputs is allowed (fball.h:24-26): x - x is
# computed on two independent balls, a ball around 0 (SPEC 4.3, docs/SPEC.md:238-240).
for (name, cfun) in ((:try_add, :p_adf_fball_add), (:try_sub, :p_adf_fball_sub), (:try_mul, :p_adf_fball_mul))
    @eval function $name(x::FBall, y::FBall)
        px = _ptr(x)
        py = _ptr(y)
        z = _new_fball(x, y)
        GC.@preserve z x y ccall($cfun[], Cvoid, (P, P, P), _ptr(z), px, py)
        return AdfResult{FBall}(OK, z)
    end
end

function try_neg(x::FBall)
    px = _ptr(x)
    z = _new_fball(x)
    GC.@preserve z x ccall(p_adf_fball_neg[], Cvoid, (P, P), _ptr(z), px)
    return AdfResult{FBall}(OK, z)
end

# Rat mixed with FBall (docs/api-j.md:209): + and - by radius-zero conversion (FBall(q), set_rat), then the
# ball operation; * by adf_fball_mul_rat, y = q x = q a + |q| N Zhat (fball.h:219-226), in both orders
# (scaling a set by a rational commutes).
for name in (:try_add, :try_sub)
    @eval function $name(x::FBall, q::Rat)
        _ptr(x)
        return $name(x, FBall(q))
    end
    @eval function $name(q::Rat, x::FBall)
        _ptr(x)
        return $name(FBall(q), x)
    end
end

function try_mul(x::FBall, q::Rat)
    px = _ptr(x)
    pq = _ptr(q)
    z = _new_fball(x)
    GC.@preserve z x q ccall(p_adf_fball_mul_rat[], Cvoid, (P, P, P), _ptr(z), px, pq)
    return AdfResult{FBall}(OK, z)
end

try_mul(q::Rat, x::FBall) = try_mul(x, q)

"""
    try_div(x::FBall, q::Rat) -> AdfResult{FBall}

`adf_fball_div_rat` (fball.h:228-232): y = (1/q) x; OK, or NOT_UNIT when `q = 0`, the fresh output cleared.
There is no division by a ball (docs/api-j.md:216-217).
"""
function try_div(x::FBall, q::Rat)
    px = _ptr(x)
    pq = _ptr(q)
    z = _new_fball(x)
    raw = GC.@preserve z x q ccall(p_adf_fball_div_rat[], Cint, (P, P, P), _ptr(z), px, pq)
    return _result(z, raw, :div)
end

# Integer and Rational scalars by explicit Rat conversion only (docs/api-j.md:232). The ball is checked live
# before the conversion allocates; a failing conversion (1//0 is DOMAIN) is returned as that status.
for name in (:try_add, :try_sub, :try_mul, :try_div)
    @eval function $name(x::FBall, y::RatScalar)
        _ptr(x)
        r = try_rat(y)
        r.status === OK || return AdfResult{FBall}(r.status, nothing)
        return $name(x, r.value)
    end
end
for name in (:try_add, :try_sub, :try_mul)
    @eval function $name(y::RatScalar, x::FBall)
        _ptr(x)
        r = try_rat(y)
        r.status === OK || return AdfResult{FBall}(r.status, nothing)
        return $name(r.value, x)
    end
end

# The Base operators throw AdfError on a status other than OK (decision J5); add, sub, mul, neg and
# divexact are the generic throwing forms of rat.jl.
const BallScalar = Union{Rat,RatScalar}

Base.:-(x::FBall) = neg(x)
for (op, f) in ((:+, :add), (:-, :sub), (:*, :mul))
    @eval Base.$op(x::FBall, y::FBall) = $f(x, y)
    @eval Base.$op(x::FBall, y::BallScalar) = $f(x, y)
    @eval Base.$op(x::BallScalar, y::FBall) = $f(x, y)
end
Base.:/(x::FBall, q::BallScalar) = divexact(x, q)
# Base defines *(::Bool, ::T) for every T <: Number (bool.jl); these resolve the ambiguity, as for Rat.
Base.:*(x::Bool, y::FBall) = mul(x, y)
Base.:*(x::FBall, y::Bool) = mul(x, y)

# ---- no == on balls; identity-based isequal and hash (docs/api-j.md:225, 234-235) ----

const NO_BALL_EQUALITY =
    "== is not defined for finite balls (docs/api-j.md:225, 234): use equal_set(x, y) for equality of the " *
    "sets, or compare_points(x, y) for the comparison of two unknown points (EQUAL, DIFFERENT, UNDECIDED)"

Base.:(==)(x::FBall, y::FBall) = throw(ArgumentError(NO_BALL_EQUALITY))
Base.:(==)(x::FBall, y::BallScalar) = throw(ArgumentError(NO_BALL_EQUALITY))
Base.:(==)(x::BallScalar, y::FBall) = throw(ArgumentError(NO_BALL_EQUALITY))

# Julia's default isequal calls == (installed help, lanes/j-slice2/help.log:26-27), which throws here; the
# handle's isequal is identity, and hash is consistent with it ("isequal(x,y) must imply that
# hash(x) == hash(y)", help.log:29-30; objectid, help.log:231-237). A Set of balls is a set of handles.
Base.isequal(x::FBall, y::FBall) = x === y
Base.isequal(x::FBall, y::BallScalar) = false
Base.isequal(x::BallScalar, y::FBall) = false
Base.hash(x::FBall, h::UInt) = hash(objectid(x), h)

# Orchestrator resolution of api-ja.md finding 19: no generic independent-ball power.
Base.:^(x::FBall, n::Integer) =
    throw(ArgumentError("^ is refused for FBall; use future pow when it lands"))
