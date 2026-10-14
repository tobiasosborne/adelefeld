# Numerical carriers (J7; docs/api-j.md:145-150,233-240). They own the whole initialized Adele/CAdele;
# no independently owned arb/acb prefix, no raw FLINT init/clear, no finitepart on their public types.

# Check the exact finite 0 using copied accessors (adele.h:129-131,200; fball.h:163-170).
# A broken internal numerical operation must fail at its boundary, before a carrier reaches the user.
function _assert_zero_fin(x::Adelic)
    f = finitepart(x)
    c = nothing
    n = nothing
    try
        c = center(f)
        n = radius(f)
        @assert string(c) == "0" && string(n) == "0" "numerical carrier finite coordinate must be exact 0"
    finally
        c === nothing || close(c)
        n === nothing || close(n)
        close(f)
    end
    return x
end

"""
    RealBall(q::Rat; prec=64)
    parse(RealBall, "1.5 +/- 0.0009765625"; prec=64)

A numerical real ball. Owns an initialized private Adele whose finite coordinate is exact 0.
The value text is the real coordinate alone. This is separate from the mathematical Adele type.
"""
struct RealBall
    _carrier::Adele
    function RealBall(::_Raw, x::Adele)
        _assert_zero_fin(x)
        return new(x)
    end
end

"""
    ComplexBall(q::Rat; prec=64)
    ComplexBall(re::RealBall, im::RealBall)
    parse(ComplexBall, "(1.5) + (-2)*i"; prec=64)

A numerical complex ball in a private initialized CAdele with finite coordinate exact 0.
The two-ball constructor copies both coordinates exactly, with no decimal intermediate.
"""
struct ComplexBall
    _carrier::CAdele
    function ComplexBall(::_Raw, x::CAdele)
        _assert_zero_fin(x)
        return new(x)
    end
end

const NumericalBall = Union{RealBall,ComplexBall}
_check_live(x::NumericalBall) = _check_live(x._carrier)

# The wrapper owns no second allocation/finalizer. Its owner's finalizer clears the whole carrier once.
Base.close(x::NumericalBall) = close(x._carrier)
_ptr(x::NumericalBall) = _ptr(x._carrier)
Base.copy(x::RealBall) = RealBall(_Raw(), copy(x._carrier))
Base.copy(x::ComplexBall) = ComplexBall(_Raw(), copy(x._carrier))
identical(x::RealBall, y::RealBall) = identical(x._carrier, y._carrier)
identical(x::ComplexBall, y::ComplexBall) = identical(x._carrier, y._carrier)
is_canonical(x::NumericalBall) = is_canonical(x._carrier)
Base.isequal(x::RealBall, y::RealBall) = x._carrier === y._carrier
Base.isequal(x::ComplexBall, y::ComplexBall) = x._carrier === y._carrier
Base.hash(x::NumericalBall, h::UInt) = hash(objectid(x._carrier), h)
Base.:(==)(x::RealBall, y::RealBall) =
    throw(ArgumentError("== is refused on numerical balls; numerical set predicates arrive with SBall"))
Base.:(==)(x::ComplexBall, y::ComplexBall) =
    throw(ArgumentError("== is refused on numerical balls; numerical set predicates arrive with SBall"))

# Getter outputs borrow only a fresh owner's first member, inside preserve, then clear the whole owner.
# No Place is needed: adele.h:119-121 has the convenience getter, and :200 has the complex counterpart.
function realpart(x::Adele)
    _check_live(x)
    z = _new_adele()
    GC.@preserve z x ccall(p_adf_adele_get_real[], Cvoid, (P, P), _ptr(z), _ptr(x))
    return RealBall(_Raw(), z)
end

function complexpart(x::CAdele)
    _check_live(x)
    z = _new_cadele()
    GC.@preserve z x ccall(p_adf_cadele_get_complex[], Cvoid, (P, P), _ptr(z), _ptr(x))
    return ComplexBall(_Raw(), z)
end

for (B, T, getter) in ((:RealBall, :Adele, :realpart), (:ComplexBall, :CAdele, :complexpart))
    @eval function $B(q::Rat; prec=64)
        _checked_prec(prec)
        _check_live(q)
        t = $T(q; prec)
        try
            return $getter(t)
        finally
            close(t)
        end
    end
    @eval function $B(q::RatScalar; prec=64)
        _checked_prec(prec)
        r = Rat(q)
        try
            return $B(r; prec)
        finally
            close(r)
        end
    end
    @eval begin
        function try_parse(::Type{$B}, s::AbstractString; prec=64,
                           limits::Union{TextLimits,Nothing}=nothing)
            _checked_prec(prec)
            r = try_parse($T, "(" * String(s) * " ; 0)"; prec, limits)
            r.status === OK || return AdfResult{$B}(r.status, nothing)
            try
                return AdfResult{$B}(OK, $B(_Raw(), r.value))
            catch
                close(r.value)
                rethrow()
            end
        end
        Base.parse(::Type{$B}, s::AbstractString; kwargs...) = _unwrap(try_parse($B, s; kwargs...), :parse)
    end
end

# FLINT numerical copy through the SAME gated libadelefeld handle's dependency. No new dlopen.
# The core has no two-arb constructor. acb_set_arb_arb is documented at
# refs/src/flint-3.0.1/acb.rst:124-126 and copies both values exactly. See api-ja.md finding 25.
function ComplexBall(re::RealBall, im::RealBall)
    _check_live(re); _check_live(im)
    z = _new_cadele()
    GC.@preserve z re im ccall(p_acb_set_arb_arb[], Cvoid, (P, P, P),
                              _ptr(z), _ptr(re._carrier), _ptr(im._carrier))
    return ComplexBall(_Raw(), z)
end

# Independent output storage obeys adele.h:112-117,190-196. C's DOMAIN is retained, never synthesized OK.
function Adele(r::RealBall, f::FBall)
    _check_live(r); _check_live(f)
    z = _new_adele()
    raw = GC.@preserve z r f ccall(p_adf_adele_set_arb_fball[], Cint, (P, P, P),
                                  _ptr(z), _ptr(r._carrier), _ptr(f))
    return _unwrap(_result(z, raw, :adele), :adele)
end

function CAdele(r::ComplexBall, f::FBall)
    _check_live(r); _check_live(f)
    z = _new_cadele()
    raw = GC.@preserve z r f ccall(p_adf_cadele_set_acb_fball[], Cint, (P, P, P),
                                  _ptr(z), _ptr(r._carrier), _ptr(f))
    return _unwrap(_result(z, raw, :cadele), :cadele)
end

# The brief's session needs this explicit injection. There is no determined numerical coordinate in
# an FBall: choose exact real 0, do not manufacture a numerical interval from its stored centre.
function Adele(f::FBall; prec=64)
    _checked_prec(prec)
    _check_live(f)
    r = RealBall(0)
    try
        return Adele(r, f)
    finally
        close(r)
    end
end

# Exactly one leading '(' and the five ASCII trailing characters ' ; 0)' are removed.
# Text construction adds exactly those same six characters. No trimming of a numerical parenthesis.
function try_value_text(x::NumericalBall; digits=20)
    r = try_value_text(x._carrier; digits)
    r.status === OK || return r
    s = r.value
    startswith(s, "(") && endswith(s, " ; 0)") || error("invalid numerical carrier text frame")
    return AdfResult{String}(OK, s[2:end-5])
end
value_text(x::NumericalBall; kwargs...) = _unwrap(try_value_text(x; kwargs...), :value_text)
Base.show(io::IO, x::NumericalBall) = print(io, value_text(x; digits=get(io, :adf_digits, 20)))

for B in (:RealBall, :ComplexBall)
    for (name, tr) in ((:add, :try_add), (:sub, :try_sub), (:mul, :try_mul))
        @eval function $tr(x::$B, y::$B; prec=64)
            _checked_prec(prec)
            _check_live(x); _check_live(y)
            r = $tr(x._carrier, y._carrier; prec)
            r.status === OK || return AdfResult{$B}(r.status, nothing)
            try
                return AdfResult{$B}(OK, $B(_Raw(), r.value))
            catch
                close(r.value)
                rethrow()
            end
        end
        @eval $name(x::$B, y::$B; kwargs...) = _unwrap($tr(x, y; kwargs...), $(QuoteNode(name)))
    end
    @eval function try_neg(x::$B)
        _check_live(x)
        r = try_neg(x._carrier)
        try
            return AdfResult{$B}(OK, $B(_Raw(), r.value))
        catch
            close(r.value)
            rethrow()
        end
    end
    @eval Base.:-(x::$B) = neg(x)
    @eval Base.:+(x::$B, y::$B) = add(x, y)
    @eval Base.:-(x::$B, y::$B) = sub(x, y)
    @eval Base.:*(x::$B, y::$B) = mul(x, y)
end
