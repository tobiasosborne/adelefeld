# Adele/CAdele: docs/api-j.md sections 3-4, step J-a.3.
# Contracts: include/adelefeld/adele.h:15-31,49-55,80-164,174-214; text.h:169-183,209-217.
# Layout: docs/api-m1.md:38-39,45. Numerical enclosure: refs/src/flint-3.0.1/arb.rst:6-26.
# Julia 1.12.5 lifetime and pointer rules: lanes/j-slice1/help.log:59-135,158-244.

# Int64 is slong on the gated ABI. Below 2 is passed unchanged; C clamps it (adele.h:21-23).
function _checked_prec(prec)
    prec isa Integer || throw(ArgumentError("prec must be an Integer fitting Int64 (slong)"))
    typemin(Int64) <= prec <= typemax(Int64) ||
        throw(ArgumentError("prec must fit Int64 (slong)"))
    return Int64(prec)
end

# text.h:54-57: printer precondition, checked before C.
function _checked_digits(digits)
    digits isa Integer && 1 <= digits <= 1_000_000 ||
        throw(ArgumentError("digits must be an Integer in 1:1000000 (text.h:54-57)"))
    return Int64(digits)
end

# Fresh handles of (real ball ; finite ball) and (complex ball ; finite ball).
# The finite coordinate is exact q. The numerical coordinate encloses q at prec bits;
# a representable dyadic is exact (adele.h:103-109,185). Only the stated methods are defined.
for (T, prefix, n, a) in ((:Adele, :adele, 96, 8), (:CAdele, :cadele, 144, 8))
    init = Symbol(:p_adf_, prefix, :_init)
    clear = Symbol(:p_adf_, prefix, :_clear)
    set = Symbol(:p_adf_, prefix, :_set)
    setrat = Symbol(:p_adf_, prefix, :_set_rat)
    canonical = Symbol(:p_adf_, prefix, :_is_canonical)
    ident = Symbol(:p_adf_, prefix, :_identical)
    getfin = Symbol(:p_adf_, prefix, :_get_fin)
    parser = Symbol(:p_adf_, prefix, :_set_str)
    printer = Symbol(:p_adf_, prefix, :_get_str)
    newfun = Symbol(:_new_, prefix)
    @eval begin
        mutable struct $T <: Number
            base::Ptr{Cvoid}
            data::Ptr{Cvoid}
            @atomic state::Int
            roots::CtxRoots  # contexts of a local finite coordinate (context.jl; adele.h:85, 130-131)
            function $T(::_Raw)
                require_loaded()
                base, data = aligned_alloc($n, $a)
                return new(base, data, STATE_NEW, nothing)
            end
        end

        # srcs: inputs of the C call that writes the value; their context roots are leased first.
        function $newfun(srcs...)
            x = $T(_Raw())
            GC.@preserve x ccall($init[], Cvoid, (P,), x.data)
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

        function _release!(x::$T)
            try
                (_, claimed) = @atomicreplace x.state STATE_LIVE => STATE_CLOSED
                claimed || return nothing
                GC.@preserve x ccall($clear[], Cvoid, (P,), x.data)
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
        Base.close(x::$T) = _release!(x)

        function _ptr(x::$T)
            (@atomic x.state) == STATE_LIVE ||
                throw(InvalidStateException("the " * $(string(T)) * " handle is closed", :closed))
            return x.data
        end

        function _result(z::$T, raw::Cint, op::Symbol)
            if raw != 0
                close(z)
                return AdfResult{$T}(to_status(raw, op), nothing)
            end
            return AdfResult{$T}(OK, z)
        end

        function $T(q::Rat; prec=64)
            p = _checked_prec(prec)
            _check_live(q)
            z = $newfun()
            GC.@preserve z q ccall($setrat[], Cvoid, (P, P, Int64), _ptr(z), _ptr(q), p)
            return z
        end
        function $T(q::RatScalar; prec=64)
            _checked_prec(prec)
            r = Rat(q)
            try
                return $T(r; prec)
            finally
                close(r)
            end
        end

        function Base.copy(x::$T)
            _check_live(x)
            z = $newfun(x)
            GC.@preserve z x ccall($set[], Cvoid, (P, P), _ptr(z), _ptr(x))
            return z
        end
        identical(x::$T, y::$T) =
            to_bool(GC.@preserve(x, y, ccall($ident[], Cint, (P, P), _ptr(x), _ptr(y))), :identical)
        is_canonical(x::$T) =
            to_bool(GC.@preserve(x, ccall($canonical[], Cint, (P,), _ptr(x))), :is_canonical)

        # Fresh copy: never independently finalize a pointer into x (adele.h:129-131,200).
        function finitepart(x::$T)
            _check_live(x)
            z = _new_fball(x)
            GC.@preserve z x ccall($getfin[], Cvoid, (P, P), _ptr(z), _ptr(x))
            return z
        end

        function try_parse(::Type{$T}, s::AbstractString; prec=64,
                           limits::Union{TextLimits,Nothing}=nothing)
            p = _checked_prec(prec)
            str = String(s)
            # Defer the C default getter until after the precision guard (api-ja.md finding 32).
            lim = Ref(limits === nothing ? TextLimits() : limits)
            z = $newfun()
            raw = GC.@preserve z str lim ccall($parser[], Cint,
                (P, Ptr{UInt8}, Csize_t, Int64, Ptr{TextLimits}), _ptr(z), pointer(str),
                ncodeunits(str), p, Base.unsafe_convert(Ptr{TextLimits}, lim))
            return _result(z, raw, :parse)
        end
        Base.parse(::Type{$T}, s::AbstractString; kwargs...) = _unwrap(try_parse($T, s; kwargs...), :parse)

        function try_value_text(x::$T; digits=20)
            d = _checked_digits(digits)
            _check_live(x)
            len = Ref{Csize_t}(0)
            GC.@preserve x len begin
                s = ccall($printer[], Ptr{UInt8}, (Ptr{Csize_t}, P, Int64),
                          Base.unsafe_convert(Ptr{Csize_t}, len), _ptr(x), d)
                try
                    (s == C_NULL || len[] == 0) && return AdfResult{String}(LIMIT, nothing)
                    unsafe_load(s + len[]) == 0x00 || error("adele printer: no NUL at s[len]")
                    return AdfResult{String}(OK, unsafe_string(s, len[]))
                finally
                    ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)
                end
            end
        end
        value_text(x::$T; kwargs...) = _unwrap(try_value_text(x; kwargs...), :value_text)
        Base.show(io::IO, x::$T) = print(io, value_text(x; digits=get(io, :adf_digits, 20)))

        Base.isequal(x::$T, y::$T) = x === y
        Base.hash(x::$T, h::UInt) = hash(objectid(x), h)
        Base.:^(x::$T, n::Integer) =
            throw(ArgumentError("^ is refused for " * $(string(T)) * "; use future pow when it lands"))
    end

    # Void ring operations synthesize OK. Every pointer used by C is produced inside preserve.
    for (name, suffix) in ((:try_add, :add), (:try_sub, :sub), (:try_mul, :mul))
        cfun = Symbol(:p_adf_, prefix, :_, suffix)
        @eval function $name(x::$T, y::$T; prec=64)
            p = _checked_prec(prec)
            _check_live(x); _check_live(y)
            z = $newfun(x, y)
            GC.@preserve z x y ccall($cfun[], Cvoid, (P, P, P, Int64),
                                    _ptr(z), _ptr(x), _ptr(y), p)
            return AdfResult{$T}(OK, z)
        end
    end
    cneg = Symbol(:p_adf_, prefix, :_neg)
    @eval function try_neg(x::$T)
        _check_live(x)
        z = $newfun(x)
        GC.@preserve z x ccall($cneg[], Cvoid, (P, P), _ptr(z), _ptr(x))
        return AdfResult{$T}(OK, z)
    end
    for (name, suffix) in ((:try_add, :add_rat), (:try_mul, :mul_rat), (:try_div, :div_rat))
        cfun = Symbol(:p_adf_, prefix, :_, suffix)
        if name === :try_div
            @eval function try_div(x::$T, q::Rat; prec=64)
                p = _checked_prec(prec)
                _check_live(x); _check_live(q)
                z = $newfun(x)
                raw = GC.@preserve z x q ccall($cfun[], Cint, (P, P, P, Int64),
                                              _ptr(z), _ptr(x), _ptr(q), p)
                return _result(z, raw, :div)
            end
        else
            @eval function $name(x::$T, q::Rat; prec=64)
                p = _checked_prec(prec)
                _check_live(x); _check_live(q)
                z = $newfun(x)
                GC.@preserve z x q ccall($cfun[], Cvoid, (P, P, P, Int64),
                                        _ptr(z), _ptr(x), _ptr(q), p)
                return AdfResult{$T}(OK, z)
            end
        end
    end

    # C has no sub_rat. Exact negation followed by add_rat keeps the scalar's finite data exact.
    @eval begin
        try_add(q::Rat, x::$T; prec=64) = try_add(x, q; prec)
        try_mul(q::Rat, x::$T; prec=64) = try_mul(x, q; prec)
        function try_sub(x::$T, q::Rat; prec=64)
            _checked_prec(prec)
            _check_live(x); _check_live(q)
            t = -q
            try
                return try_add(x, t; prec)
            finally
                close(t)
            end
        end
        function try_sub(q::Rat, x::$T; prec=64)
            _checked_prec(prec)
            _check_live(x); _check_live(q)
            t = neg(x)
            try
                return try_add(t, q; prec)
            finally
                close(t)
            end
        end
    end
    for name in (:try_add, :try_sub, :try_mul, :try_div)
        @eval function $name(x::$T, q::RatScalar; prec=64)
            _checked_prec(prec)
            _check_live(x)
            r = try_rat(q)
            r.status === OK || return AdfResult{$T}(r.status, nothing)
            try
                return $name(x, r.value; prec)
            finally
                close(r.value)
            end
        end
    end
    for name in (:try_add, :try_sub, :try_mul)
        @eval function $name(q::RatScalar, x::$T; prec=64)
            _checked_prec(prec)
            _check_live(x)
            r = try_rat(q)
            r.status === OK || return AdfResult{$T}(r.status, nothing)
            try
                return $name(r.value, x; prec)
            finally
                close(r.value)
            end
        end
    end
end

const Adelic = Union{Adele,CAdele}

# Julia 1.12.5 help, lanes/j-slice3/help.log:272-279: negative literals otherwise call inv first.
# Keep one ^ method per type and route every literal exponent through those same refusals.
Base.literal_pow(::typeof(^), x::Union{FBall,Adelic}, ::Val{n}) where {n} = x^n

# Early lifecycle checks return no pointer. Every numerical address is produced inside preserve.
function _check_live(x::Union{Rat,FBall,Adelic})
    (@atomic x.state) == STATE_LIVE ||
        throw(InvalidStateException("the $(typeof(x)) handle is closed", :closed))
    return nothing
end

# Exact embedding, not a text conversion (adele.h:187-188).
function CAdele(x::Adele)
    _check_live(x)
    z = _new_cadele(x)
    GC.@preserve z x ccall(p_adf_cadele_set_adele[], Cvoid, (P, P), _ptr(z), _ptr(x))
    return z
end

for name in (:try_add, :try_sub, :try_mul)
    @eval function $name(x::Adele, y::CAdele; prec=64)
        _checked_prec(prec)
        _check_live(x); _check_live(y)
        t = CAdele(x)
        try
            return $name(t, y; prec)
        finally
            close(t)
        end
    end
    @eval function $name(x::CAdele, y::Adele; prec=64)
        _checked_prec(prec)
        _check_live(x); _check_live(y)
        t = CAdele(y)
        try
            return $name(x, t; prec)
        finally
            close(t)
        end
    end
end

# Typed named forms add keywords without changing the precision-free Rat/FBall methods.
for (name, tr) in ((:add, :try_add), (:sub, :try_sub), (:mul, :try_mul))
    @eval $name(x::Adelic, y::Union{Adelic,BallScalar}; kwargs...) =
        _unwrap($tr(x, y; kwargs...), $(QuoteNode(name)))
    @eval $name(x::BallScalar, y::Adelic; kwargs...) = _unwrap($tr(x, y; kwargs...), $(QuoteNode(name)))
end
divexact(x::Adelic, q::BallScalar; kwargs...) = _unwrap(try_div(x, q; kwargs...), :divexact)

for (op, name) in ((:+, :add), (:-, :sub), (:*, :mul))
    @eval Base.$op(x::Adelic, y::Union{Adelic,BallScalar}) = $name(x, y)
    @eval Base.$op(x::BallScalar, y::Adelic) = $name(x, y)
end
Base.:-(x::Adelic) = neg(x)
Base.:/(x::Adelic, q::BallScalar) = divexact(x, q)
Base.:*(x::Bool, y::Adelic) = mul(x, y)
Base.:*(x::Adelic, y::Bool) = mul(x, y)

# Explicit refusal avoids Number promotion's ErrorException for the unsupported Number families.
const AdelicUnsupported = Union{FBall,AbstractFloat,Complex}
const NO_ADELE_MIX = "unsupported adelic pair; use explicit numerical coordinates and an FBall constructor"
for op in (:+, :-, :*)
    @eval Base.$op(x::Adelic, y::AdelicUnsupported) = throw(ArgumentError(NO_ADELE_MIX))
    @eval Base.$op(x::AdelicUnsupported, y::Adelic) = throw(ArgumentError(NO_ADELE_MIX))
end
Base.:/(x::Adelic, y::Union{Adelic,AdelicUnsupported}) = throw(ArgumentError("divide by an exact Rat only"))
Base.:/(x::Union{BallScalar,AdelicUnsupported}, y::Adelic) =
    throw(ArgumentError("division by an Adele or CAdele is undefined"))

const AdelicComparable = Union{Adelic,BallScalar,AdelicUnsupported}
const NO_ADELE_EQUALITY = "== is refused on adelic balls; compare finitepart with equal_set/compare_points; " *
                          "numerical predicates arrive with SBall"
Base.:(==)(x::Adelic, y::AdelicComparable) = throw(ArgumentError(NO_ADELE_EQUALITY))
Base.:(==)(x::Union{BallScalar,AdelicUnsupported}, y::Adelic) = throw(ArgumentError(NO_ADELE_EQUALITY))
Base.isequal(x::Adelic, y::Union{BallScalar,AdelicUnsupported}) = false
Base.isequal(x::Union{BallScalar,AdelicUnsupported}, y::Adelic) = false
Base.isequal(x::Adele, y::CAdele) = false
Base.isequal(x::CAdele, y::Adele) = false
