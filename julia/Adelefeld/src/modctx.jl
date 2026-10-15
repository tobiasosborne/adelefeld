# Modulus contexts and the local backend: step 1 of J-b (docs/api-j.md:249-260, section 9).
# C contract: include/adelefeld/modctx.h:11-35 (rules of every constructor: *out written only on OK, the
# caller owns the context, arrays copied, at most ADF_MODCTX_MAX_BLOCKS blocks, DOMAIN before UNSUPPORTED),
# :57-87 (constructors), :99-101 (free), :105-112 (accessors), :145-174 (conversions of adf_fball).
# Lifetime: context.jl (the context proof of docs/api-j.md:126-141).
# fmpz arguments and outputs use the numerator of a Rat with denominator 1 (docs/api-j.md:148-149): an fmpq is
# "a pair of fmpz's representing numerator and denominator", whose fields a user may set when the result
# stays canonical (refs/src/flint-3.0.1/fmpq.rst:9-11, 21-26, 39-41; docs/api-m1.md:35).

const MODCTX_FAMILIES = (:blocks, :prime_powers, :modulus, :factorial, :primorial_power)

# A Julia Integer as a C ulong word: an explicit ArgumentError outside 0..2^64-1, never a wrapped word
# (docs/api-j.md:154-155).
function _word(v, what::String)
    v isa Integer || throw(ArgumentError("$what must be an Integer, got $(typeof(v))"))
    0 <= v <= typemax(UInt64) || throw(ArgumentError("$what must be in 0:2^64-1 (a C ulong), got $v"))
    return UInt64(v)
end

function _words(v, what::String)
    v isa AbstractVector || throw(ArgumentError("$what must be a vector of Integers"))
    return UInt64[_word(e, what * " element") for e in v]     # a Julia-owned copy
end

# The handle of a context just written by a constructor that returned OK (context proof step 1).
function _own_modctx(ptr::Ptr{Cvoid})
    ptr == C_NULL && error("modctx constructor returned OK with a NULL context (modctx.h:12-13)")
    s = CtxState(ptr, 1)
    Threads.atomic_add!(CTX_LIVE, 1)
    c = ModCtx(_Raw(), s)
    try
        finalizer(_release_owner!, c)
    catch
        _release_owner!(c)
        rethrow()
    end
    return c
end

"""
    try_modctx(; blocks, prime_powers, modulus, factorial, primorial_power) -> AdfResult{ModCtx}
    ModCtx(; kwargs...) -> ModCtx

A new modulus context from EXACTLY ONE construction family (docs/api-j.md:255); none or two or more is an
`ArgumentError` before any C call. Arguments are checked and copied into Julia-owned `Vector{UInt64}`
before C (C copies them again, modctx.h:20); every word must fit a C ulong (ArgumentError otherwise).

- `blocks=[q1, ..., qk]`: `adf_modctx_new_blocks` (modctx.h:57-62). DOMAIN unless every q >= 2 and the q
  are pairwise coprime; UNSUPPORTED if k > 65536 (modctx.h:23-28, 48).
- `prime_powers=[p1 => e1, ...]`: `adf_modctx_new_prime_powers` (:64-69). DOMAIN if some p is not prime, a
  prime repeats or some e = 0; UNSUPPORTED if some p^e >= 2^64 or k > 65536.
- `modulus=K`: `adf_modctx_new_fmpz` (:71-74). DOMAIN if K < 1; one block if 2 <= K < 2^64; no block if
  K = 1 or K >= 2^64.
- `factorial=n`: `adf_modctx_new_factorial` (:76-81). UNSUPPORTED if a prime power of n! is >= 2^64.
- `primorial_power=(n, e)`: `adf_modctx_new_primorial_pow` (:83-87). UNSUPPORTED if some p^e >= 2^64, or
  n >= 821647 (more than 65536 blocks, modctx.h:26-28, 45-49).

Blocks are kept in the order supplied; k = 0, n < 2 or e = 0 give the modulus 1 without blocks, a context
that serves only the scaled policy. DOMAIN is decided before UNSUPPORTED (modctx.h:31-32). No constructor
returns LIMIT. Only OK reads the C pointer slot (initialized NULL) and creates a handle with its owner token;
every status has `details = (family = <keyword>,)`. For `modulus=K` the integer is first made a `Rat`; a
failure of that conversion (LIMIT for a text above the default limits) is returned as its status.
Cost (modctx.h:60-61): blocks k^2 word gcds and the tables of the context; the others enumerate primes up to
n or test k primes, then build the tables.
"""
function try_modctx(; blocks=nothing, prime_powers=nothing, modulus=nothing, factorial=nothing,
                    primorial_power=nothing)
    vals = (blocks, prime_powers, modulus, factorial, primorial_power)
    given = [MODCTX_FAMILIES[i] for i in 1:5 if vals[i] !== nothing]
    length(given) == 1 || throw(ArgumentError(
        "try_modctx needs exactly one construction family of $(MODCTX_FAMILIES); got $(Tuple(given))"))
    fam = given[1]
    details = (family = fam,)
    require_loaded()
    out = Ref{Ptr{Cvoid}}(C_NULL)    # the pointer slot; read only on OK (modctx.h:12-16)
    if fam === :blocks
        q = _words(blocks, "blocks")
        raw = GC.@preserve q out ccall(p_adf_modctx_new_blocks[], Cint, (Ptr{Ptr{Cvoid}}, Ptr{UInt64}, Int64),
                                       out, q, length(q))
    elseif fam === :prime_powers
        prime_powers isa AbstractVector && all(e -> e isa Pair, prime_powers) ||
            throw(ArgumentError("prime_powers must be a vector of pairs p => e"))
        p = UInt64[_word(pe.first, "prime") for pe in prime_powers]
        e = UInt64[_word(pe.second, "exponent") for pe in prime_powers]
        raw = GC.@preserve p e out ccall(p_adf_modctx_new_prime_powers[], Cint,
                                         (Ptr{Ptr{Cvoid}}, Ptr{UInt64}, Ptr{UInt64}, Int64), out, p, e, length(p))
    elseif fam === :modulus
        modulus isa Integer || throw(ArgumentError("modulus must be an Integer, got $(typeof(modulus))"))
        r = try_rat(modulus)
        r.status === OK || return AdfResult{ModCtx}(r.status, nothing, nothing, details)
        K = r.value
        try
            raw = GC.@preserve K out ccall(p_adf_modctx_new_fmpz[], Cint, (Ptr{Ptr{Cvoid}}, P), out, _ptr(K))
        finally
            close(K)
        end
    elseif fam === :factorial
        n = _word(factorial, "factorial")
        raw = ccall(p_adf_modctx_new_factorial[], Cint, (Ptr{Ptr{Cvoid}}, UInt64), out, n)
    else
        primorial_power isa Tuple{Integer,Integer} ||
            throw(ArgumentError("primorial_power must be a tuple (n, e) of two Integers"))
        n = _word(primorial_power[1], "primorial n")
        e = _word(primorial_power[2], "primorial exponent")
        raw = ccall(p_adf_modctx_new_primorial_pow[], Cint, (Ptr{Ptr{Cvoid}}, UInt64, UInt64), out, n, e)
    end
    st = to_status(raw, :modctx)
    st === OK || return AdfResult{ModCtx}(st, nothing, nothing, details)
    return AdfResult{ModCtx}(OK, _own_modctx(out[]), nothing, details)
end

ModCtx(; kwargs...) = _unwrap(try_modctx(; kwargs...), :modctx)

"""
    modulus(ctx::ModCtx) -> Rat

The modulus K of the context, through `adf_modctx_get_modulus` (modctx.h:105-106), as an exact integer
`Rat`. A `Rat` and not a `BigInt`: the C output is an fmpz, written here into the numerator of a fresh
`Rat` whose denominator stays 1 (K >= 1, so the value is canonical); a `BigInt` would need either a
reinterpretation of GMP storage, which the design forbids (docs/api-j.md:196), or a text round trip.
`modulus(ctx) == K` compares with a Julia Integer exactly. Never fails; cost a copy of K.
"""
function modulus(c::ModCtx)
    pc = _cptr(c)
    K = _new_rat()
    GC.@preserve K c ccall(p_adf_modctx_get_modulus[], Cvoid, (P, P), _ptr(K), pc)
    return K
end

"""
    nblocks(ctx::ModCtx) -> Int

The number k >= 0 of word blocks (`adf_modctx_nblocks`, modctx.h:108-109).
"""
nblocks(c::ModCtx) = Int(GC.@preserve c ccall(p_adf_modctx_nblocks[], Int64, (P,), _cptr(c)))

"""
    blocks(ctx::ModCtx) -> Vector{UInt64}

The blocks q_1, ..., q_k in their order (`adf_modctx_block` for 0 <= i < k, modctx.h:111-112).
"""
function blocks(c::ModCtx)
    k = nblocks(c)
    return GC.@preserve c UInt64[ccall(p_adf_modctx_block[], UInt64, (P, Int64), _cptr(c), i) for i in 0:k-1]
end

function Base.show(io::IO, c::ModCtx)
    if !(@atomic c.owner)
        print(io, "ModCtx(finalized)")
        return
    end
    K = modulus(c)
    try
        b = blocks(c)
        shown = join(string.(b[1:min(end, 10)]), ", ")
        length(b) > 10 && (shown *= ", ... ($(length(b)) blocks)")
        print(io, "ModCtx(modulus=", value_text(K), ", blocks=[", shown, "])")
    finally
        close(K)
    end
end

# ---- the local backend of adf_fball (modctx.h:143-174; conventions 5.3) ----

"""
    try_to_local(x::FBall, ctx::ModCtx) -> AdfResult{FBall}
    to_local(x, ctx) -> FBall

The set of `x` as a local value of `ctx`, exactly (`adf_fball_set_local`, modctx.h:145-153): the ball
c + R Zhat, R > 0, is local in a context of modulus K exactly when K/R and c K/R are integers. OK gives a
fresh local FBall that holds a lease on `ctx` and `details = (lost = false,)` (the conversion is exact).
DOMAIN if the set is not a local value of ctx, an exact value included; UNSUPPORTED if ctx has no block.
On failure there is no value and `details` is empty. A closed `x` or a finalized `ctx` is refused before C.
Cost: one division and k word reductions.
"""
function try_to_local(x::FBall, ctx::ModCtx)
    _check_live(x)
    pc = _cptr(ctx)          # refused before C if finalized; the lease below keeps pc valid
    z = _new_fball(x, ctx)
    raw = GC.@preserve z x ctx ccall(p_adf_fball_set_local[], Cint, (P, P, P), _ptr(z), _ptr(x), pc)
    r = _result(z, raw, :to_local)
    r.status === OK || return r
    return AdfResult{FBall}(OK, z, nothing, (lost = false,))
end

"""
    try_enclose_local(x::FBall, ctx::ModCtx) -> AdfResult{FBall}
    enclose_local(x, ctx) -> FBall

The best enclosure of `x` among the local values of `ctx` (`adf_fball_set_local_enclose`,
modctx.h:155-164): with K/R = n/m in lowest terms and e the denominator of a rational point of x,
d0 = lcm(n, e) and y = (c d0 + K Zhat)/d0. OK gives a fresh local FBall with a lease on `ctx` and
`details = (lost = <Bool>,)`, true when the set changed (d0 != K/R). The C `lost` output starts as an
invalid sentinel and is read ONLY on OK (docs/api-j.md:258; modctx.h:160). DOMAIN if x is exact, UNSUPPORTED
if ctx has no block; then no value and empty details.
"""
function try_enclose_local(x::FBall, ctx::ModCtx)
    _check_live(x)
    pc = _cptr(ctx)
    z = _new_fball(x, ctx)
    lost = Ref{Cint}(-1)
    raw = GC.@preserve z x ctx lost ccall(p_adf_fball_set_local_enclose[], Cint, (P, Ptr{Cint}, P, P),
                                          _ptr(z), Base.unsafe_convert(Ptr{Cint}, lost), _ptr(x), pc)
    r = _result(z, raw, :enclose_local)
    r.status === OK || return r
    l = try
        to_bool(lost[], :enclose_local)
    catch
        _release!(z)
        rethrow()
    end
    return AdfResult{FBall}(OK, z, nothing, (lost = l,))
end

"""
    try_to_global(x::FBall) -> AdfResult{FBall}
    to_global(x::FBall) -> FBall

`x` in the global backend (`adf_fball_set_global`, modctx.h:166-169): recombination by CRT and canonical
cancellation; a global x is copied; the set is unchanged. Statusless: OK is synthesized. The result leases
the contexts of `x` conservatively (docs/api-j.md:133-134), until it is cleared.
"""
function try_to_global(x::FBall)
    _check_live(x)
    z = _new_fball(x)
    GC.@preserve z x ccall(p_adf_fball_set_global[], Cvoid, (P, P), _ptr(z), _ptr(x))
    return AdfResult{FBall}(OK, z)
end

to_local(x::FBall, ctx::ModCtx) = _unwrap(try_to_local(x, ctx), :to_local)
enclose_local(x::FBall, ctx::ModCtx) = _unwrap(try_enclose_local(x, ctx), :enclose_local)
to_global(x::FBall) = _unwrap(try_to_global(x), :to_global)

"""
    is_local(x::FBall) -> Bool

`adf_fball_is_local` (modctx.h:171-173): the backend is ADF_LOCAL. Not part of the value text, which is the
same for both backends (conventions 9.8 A11; text.h:37-38).
"""
is_local(x::FBall) = to_bool(GC.@preserve(x, ccall(p_adf_fball_is_local[], Cint, (P,), _ptr(x))), :is_local)

"""
    Adelefeld.context_pointer(x::FBall) -> Ptr{Cvoid}

Private: `adf_fball_context` (modctx.h:171-174), the borrowed context pointer, C_NULL for a global value.
"""
context_pointer(x::FBall) = GC.@preserve x ccall(p_adf_fball_context[], Ptr{Cvoid}, (P,), _ptr(x))

"""
    context(x::FBall) -> Union{ModCtx,Nothing}

The context a local value borrows, found by comparing `adf_fball_context(x)` (modctx.h:171-174) with the
pointers of the contexts `x` holds leases on; `nothing` for a global value. A pointer outside the leases
would be a broken lease invariant and throws.
"""
function context(x::FBall)
    p = context_pointer(x)
    p == C_NULL && return nothing
    r = x.roots
    if r !== nothing
        for m in r
            m.state.ptr == p && return m
        end
    end
    error("lease invariant broken: the FBall borrows a context it holds no lease on")
end
