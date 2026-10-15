# Context states, tokens and leases: the context proof of docs/api-j.md:126-141 (decision J6, :26-27),
# implementing docs/conventions.md:342-397 (contexts are caller-owned with a stated lifetime: free only after
# every value that refers to the context has been cleared) and :1582-1583 ("the value object holds a
# reference to the context object"). C contract: include/adelefeld/modctx.h:11-22 (the caller owns the
# context; freeing a borrowed context is undefined behaviour), :99-101 (adf_modctx_free).
# Julia 1.12.5 help: @atomicreplace returns "(old, success::Bool)" (lanes/j-slice1/help.log:158-230);
# modifyfield! returns the pair old => new (observed in lanes/j-slice5/probe.log, line 2).
#
# The proof, step by step (numbers as in docs/api-j.md:127-136):
# 1. A successful constructor creates one CtxState (ptr, count = 1); that token is the owner token, held by
#    the one ModCtx handle (field `owner`). A finalized ModCtx is refused as an explicit argument (_cptr).
# 2. Before a value can borrow, _lease! takes one token per DISTINCT state of its sources (input values and
#    explicit contexts) and stores the ModCtx handles in the value's roots. A failure while taking tokens
#    gives back the tokens taken; a failing C call afterwards releases the value, which gives them back.
# 3. A fresh result leases the union of its sources' roots before the C call. No C operation creates a
#    context (modctx.h:20-21), so every context pointer a result can hold is in that union. The union is
#    conservative: a result that fell back to global keeps its roots until it is cleared (:133-134).
# 4. A value drops its tokens only after its whole C clear and the free of its storage (_drop_roots!,
#    called last by every _release!). The ModCtx finalizer drops the owner token (_release_owner!).
# 5. _drop! decrements atomically; only the decrement that sees the old count 1 calls adf_modctx_free.
#    When it runs, the owner token and every value token are gone, so every borrower has cleared. Exactly
#    one decrement sees 1, so exactly one thread frees, whatever the order of the finalizers.
# Taking a token (_acquire!) needs count >= 1 and is a compare-and-swap that never revives count 0: the
# caller always holds a source that itself holds a token (a live ModCtx owner, or a live value's lease).

const CTX_LIVE = Threads.Atomic{Int}(0)    # contexts constructed minus contexts freed
const CTX_FREES = Threads.Atomic{Int}(0)   # calls of adf_modctx_free

"""
    CtxState

Private: the C context pointer and the atomic token count (docs/api-j.md:127). Never exposed.
"""
mutable struct CtxState
    const ptr::Ptr{Cvoid}
    @atomic count::Int
end

"""
    ModCtx

A modulus context (include/adelefeld/modctx.h; conventions 5.14): an immutable C context made by one of the
explicit constructors of `try_modctx`. The handle owns one token of the private state; values that borrow
the context hold their own tokens and keep the handle in their roots, so the C context is freed only after
the handle is finalized and every borrowing value is cleared (docs/api-j.md:126-141). There is no public
close in 0.1 (docs/api-j.md:137); `finalize(ctx)` drops the owner token early, after which `ctx` is refused
as an argument while existing values stay valid. Equal moduli are not equal contexts: no interning
(docs/api-j.md:253; conventions 4.6, last paragraph).
"""
mutable struct ModCtx
    const state::CtxState
    @atomic owner::Bool
    ModCtx(::_Raw, s::CtxState) = new(s, true)
end

const CtxRoots = Union{Nothing,Vector{ModCtx}}

# Take one token of s. Refuses a state whose count is 0 (already freed), never reviving it.
function _acquire!(s::CtxState)
    c = @atomic s.count
    while true
        c >= 1 || throw(InvalidStateException("the modulus context was freed", :closed))
        (c, ok) = @atomicreplace s.count c => c + 1
        ok && return nothing
    end
end

# Drop one token of s; the decrement that sees the old count 1 frees the C context (step 5).
# A count below 1 before the decrement is a broken invariant: it throws, and the caller counts it.
function _drop!(s::CtxState)
    old = modifyfield!(s, :count, -, 1, :sequentially_consistent).first
    old >= 1 || error("modulus context token count was $old before a drop")
    if old == 1
        ccall(p_adf_modctx_free[], Cvoid, (P,), s.ptr)
        Threads.atomic_sub!(CTX_LIVE, 1)
        Threads.atomic_add!(CTX_FREES, 1)
    end
    return nothing
end

# The ModCtx finalizer (step 4): claims the owner token once and drops it. No I/O, no task switch.
function _release_owner!(c::ModCtx)
    try
        (_, claimed) = @atomicreplace c.owner true => false
        claimed || return nothing
        _drop!(c.state)
    catch
        Threads.atomic_add!(FINALIZER_ERRORS, 1)
    end
    return nothing
end

# The C pointer of a context whose owner token is held; a finalized ModCtx is refused before C (step 1).
function _cptr(c::ModCtx)
    (@atomic c.owner) || throw(InvalidStateException("the ModCtx was finalized", :closed))
    return c.state.ptr
end

_roots_of(x) = x.roots
_roots_of(::Rat) = nothing

function _addroot(acc::CtxRoots, m::ModCtx)
    acc === nothing && return ModCtx[m]
    any(r -> r === m, acc) || push!(acc, m)
    return acc
end

# Step 2/3: the union of the roots of srcs (values, or explicit ModCtx arguments), one token per distinct
# state, stored in z.roots before z is passed to C. On failure the tokens taken are given back, z is
# released (it is a fresh owner without roots) and the exception is rethrown.
function _lease!(z, srcs...)
    acc = nothing
    for s in srcs
        if s isa ModCtx
            acc = _addroot(acc, s)
        else
            r = _roots_of(s)
            r === nothing && continue
            for m in r
                acc = _addroot(acc, m)
            end
        end
    end
    acc === nothing && return z
    taken = 0
    try
        for m in acc
            _acquire!(m.state)
            taken += 1
        end
    catch
        for i in 1:taken
            _drop!(acc[i].state)
        end
        _release!(z)
        rethrow()
    end
    z.roots = acc
    return z
end

# Step 4: called by every _release! after the C clear and the free of the storage.
function _drop_roots!(x)
    r = x.roots
    r === nothing && return nothing
    x.roots = nothing
    for m in r
        _drop!(m.state)
    end
    return nothing
end

"""
    Adelefeld.roots(x) -> Vector{ModCtx}

Private diagnostic: a copy of the contexts `x` holds tokens on (empty for a value without leases).
"""
roots(x) = (r = _roots_of(x); r === nothing ? ModCtx[] : copy(r))

"""
    Adelefeld.ctx_tokens(ctx) -> Int

Private diagnostic: the token count of the context state (owner token plus value tokens; 0 once freed).
"""
ctx_tokens(c::ModCtx) = @atomic c.state.count

"""
    Adelefeld.ctx_pointer(ctx) -> Ptr{Cvoid}

Private diagnostic: the C context pointer, for comparison with `context_pointer(x)` in tests only.
"""
ctx_pointer(c::ModCtx) = c.state.ptr
