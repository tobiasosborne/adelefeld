# Heap-handle prototype. No package code, C glue, header parser or external Julia dependency.
# C sources: include/adelefeld/rat.h:40-60,100-116; text.h:147-154; common.h:66-70.
using Libdl

length(ARGS) == 1 || error("usage: probe.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(abspath(ARGS[1]))  # Kept open until process exit, including finalizers.
sym(s) = Libdl.dlsym(LIB, s)
const INIT = sym(:adf_rat_init)
const CLEAR = sym(:adf_rat_clear)
const SETSTR = sym(:adf_rat_set_str)
const GETSTR = sym(:adf_rat_get_str)
const STRFREE = sym(:adf_str_free)
const ADD = sym(:adf_rat_add)
const MUL = sym(:adf_rat_mul)
const CANON = sym(:adf_rat_is_canonical)
const STATUS = sym(:adf_status_str)
const P = Ptr{Cvoid}
const SIZE = ccall(sym(:adf_sizeof_rat), Csize_t, ())
const ALIGN = ccall(sym(:adf_alignof_rat), Csize_t, ())
@assert (SIZE, ALIGN) == (16, 8)
@assert ccall(sym(:adf_version_check), Cint, ()) == 0
const inits = Ref(0)
const clears = Ref(0)
const allocs = Ref(0)
const frees = Ref(0)
const string_allocs = Ref(0)
const string_frees = Ref(0)
const aligns = Ref(0)
const arithmetic = Ref(0)
const equalities = Ref(0)
const canonicals = Ref(0)
const gc_calls = Ref(0)

mutable struct ProbeRat
    base::P
    data::P
    live::Bool
end

# No I/O, task switching, callback to Julia from C, or status-producing call in a finalizer.
# This prototype is single-threaded. Package counters require atomics; see the design.
function release!(x::ProbeRat)
    x.live || return nothing
    GC.@preserve x begin
        ccall(CLEAR, Cvoid, (P,), x.data)
        clears[] += 1
        Libc.free(x.base)
        frees[] += 1
        x.live = false
        x.base = C_NULL
        x.data = C_NULL
    end
    return nothing
end

function ProbeRat()
    base = Libc.malloc(SIZE + ALIGN - 1)
    base == C_NULL && error("probe allocation failed")
    allocs[] += 1
    # Align inside the allocation; free the base, never the adjusted pointer.
    data = P((UInt(base) + ALIGN - 1) & ~(UInt(ALIGN) - 1))
    @assert UInt(data) % ALIGN == 0
    aligns[] += 1
    x = ProbeRat(base, data, false)
    GC.@preserve x ccall(INIT, Cvoid, (P,), x.data)
    x.live = true
    inits[] += 1
    try
        finalizer(release!, x)
    catch
        release!(x)
        rethrow()
    end
    return x
end

function set_text!(x, text)
    st = GC.@preserve x text ccall(SETSTR, Cint, (P, Ptr{UInt8}, Csize_t, P),
                                  x.data, pointer(text), sizeof(text), C_NULL)
    st == 0 || error("parse status ", unsafe_string(ccall(STATUS, Ptr{UInt8}, (Cint,), st)))
end

function square_double!(x)
    GC.@preserve x begin
        ccall(ADD, Cvoid, (P, P, P), x.data, x.data, x.data)
        ccall(MUL, Cvoid, (P, P, P), x.data, x.data, x.data)
        arithmetic[] += 2
        @assert ccall(CANON, Cint, (P,), x.data) == 1
        canonicals[] += 1
    end
end

function value_text(x)
    n = Ref{Csize_t}(0)
    GC.@preserve x n begin
        p = ccall(GETSTR, Ptr{UInt8}, (Ref{Csize_t}, P), n, x.data)
        @assert p != C_NULL
        string_allocs[] += 1
        try
            @assert unsafe_load(p + n[]) == 0
            return unsafe_string(p, n[])
        finally
            ccall(STRFREE, Cvoid, (Ptr{UInt8},), p)
            string_frees[] += 1
        end
    end
end

function collect_now()
    GC.gc()
    gc_calls[] += 1
end

function batch(first, count)
    xs = ProbeRat[]
    sizehint!(xs, count)
    den0 = (big(1) << 127) - 1
    for i in first:first + count - 1
        x = ProbeRat()
        num0 = (big(1) << 150) + i
        set_text!(x, string(num0, '/', den0))
        push!(xs, x)
    end
    collect_now()  # All handles must survive while reachable through xs.
    for (j, x) in enumerate(xs)
        i = first + j - 1
        num0 = (big(1) << 150) + i
        oracle = (2 * (num0 // den0))^2
        square_double!(x)
        wanted = denominator(oracle) == 1 ? string(numerator(oracle)) :
                 string(numerator(oracle), '/', denominator(oracle))
        @assert value_text(x) == wanted
        equalities[] += 1
    end
    # Explicit repeated finalization must not double-clear the first handle.
    before = clears[]
    finalize(xs[1])
    finalize(xs[1])
    @assert clears[] == before + 1
    return nothing  # No handle escapes this function.
end

function main()
    started = time_ns()
    for k in 0:9
        batch(10000k + 1, 10000)
        for _ in 1:3
            collect_now()
        end
        @assert inits[] == clears[] == allocs[] == frees[] == (k + 1) * 10000
        println("batch=", k + 1, " init=", inits[], " clear=", clears[], " free=", frees[])
    end
    elapsed = (time_ns() - started) / 1e9
    println("Julia=", VERSION, " sizeof=", SIZE, " alignof=", ALIGN)
    println("init=", inits[], " clear=", clears[], " malloc=", allocs[], " free=", frees[])
    println("live_handles=", inits[] - clears[], " live_buffers=", allocs[] - frees[])
    println("aligned=", aligns[], " arithmetic_calls=", arithmetic[], " canonical_checks=", canonicals[])
    println("oracle_equalities=", equalities[], " strings=", string_allocs[], " string_frees=", string_frees[])
    println("gc_calls=", gc_calls[], " repeated_finalize_checks=10 elapsed_seconds=", round(elapsed; digits=3))
end

main()
