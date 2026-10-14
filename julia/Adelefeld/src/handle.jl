# Owned C storage (decision J4 at docs/api-j.md:21-23; section 3, lines 100-114).
# Julia 1.12.5 help (lanes/j-slice1/help.log): Libc.malloc (231-237); Libc.free "Only use this on memory
# obtained from malloc" (238-244); Threads.Atomic (346-373); Base.Checked.checked_add (374-382).

const STATE_NEW = 0       # storage allocated, adf init not yet run
const STATE_LIVE = 1      # adf init ran; the value may be used
const STATE_CLOSED = 2    # clear ran and the storage is freed, or release was claimed

const LIVE_HANDLES = Threads.Atomic{Int}(0)       # adf inits minus adf clears, over all handles
const FINALIZER_ERRORS = Threads.Atomic{Int}(0)   # unexpected exceptions inside a release (:111-112)

"""
    aligned_alloc(n, a) -> (base, p)

Allocates `n + a - 1` bytes with `Libc.malloc` after checked size arithmetic and returns the base pointer
(the one to free) and the aligned pointer `p` (docs/api-j.md:100-102).

Proof of alignment and containment, for a power of two `a` and the base address `b` returned by malloc for
`t = n + a - 1` bytes (so that `b + t` does not wrap: the object [b, b + t) exists):
1. `p = (b + a - 1) & ~(a - 1)` clears the low log2(a) bits, so `p` is a multiple of `a`: `p % a == 0`.
2. Clearing low bits subtracts `(b + a - 1) mod a`, which is at most `a - 1`; so `p >= b`.
3. Clearing bits never increases a number; so `p <= b + a - 1`, and `p + n <= b + a - 1 + n = b + t`.
Hence [p, p + n) lies inside [b, b + t) and `p` is aligned, whatever alignment malloc itself gives. The
checks below test exactly 1 and 3 at run time; a failure frees the base and throws.
"""
function aligned_alloc(n::Int, a::Int)
    (n > 0 && a > 0 && ispow2(a)) || throw(ArgumentError("aligned_alloc: size $n, alignment $a"))
    t = Base.Checked.checked_add(n, a - 1)
    base = Libc.malloc(t)
    base == C_NULL && throw(OutOfMemoryError())
    b = UInt(base)
    p = (b + UInt(a - 1)) & ~UInt(a - 1)
    if !(p % UInt(a) == 0 && p >= b && p + UInt(n) <= b + UInt(t))
        Libc.free(base)
        error("aligned_alloc: the alignment proof failed at run time (base $b, size $n, alignment $a)")
    end
    return base, Ptr{Cvoid}(p)
end

"""
    Adelefeld.diagnostics()

The load state, the library path, the number of live handles (adf inits minus clears) and the number of
unexpected exceptions caught in a release (a leak; docs/api-j.md:111-112).
"""
diagnostics() = (state = LOAD_STATE[], library = LIBPATH[], live = LIVE_HANDLES[],
                 finalizer_errors = FINALIZER_ERRORS[])
