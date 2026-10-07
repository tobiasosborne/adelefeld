# Slice 3.1-f. Negation and addition of quotient classes (docs/api-3.md 2.4, Q3, section 7 calls).
using Test
using Libdl

length(ARGS) == 1 || error("usage: qclass_arith.jl <libadelefeld.so>")
const libadf = ARGS[1]
const LIB = Libdl.dlopen(libadf)
ad(name) = Libdl.dlsym(LIB, name)

function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    ptr = Ptr{Cvoid}((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, ptr
end

function readclass(q, text)
    GC.@preserve text begin
        @test ccall(ad(:adf_qclass_set_str), Cint,
            (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}),
            q, text, sizeof(text), 128, C_NULL) == 0
    end
end

function printed(q)
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_qclass_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}, Clong), n, q, 2)
    @test s != C_NULL
    s == C_NULL && return ""
    try
        return unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (Ptr{Cvoid},), s)
    end
end

identical(a, b) = ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, b) == 1

@testset "qclass add and neg" begin
    qb, q = storage("qclass")
    q2b, q2 = storage("qclass")
    yb, y = storage("qclass")
    sb, saved = storage("qclass")
    GC.@preserve qb q2b yb sb begin
        for p in (q, q2, y, saved)
            ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), p)
        end
        try
            readclass(q, "(0.75 +/- 0.25 ; 0 mod 3) + Q")
            readclass(q2, "(0.5 +/- 0.5 ; 1 mod 3) + Q")
            lim = 2
            @test ccall((:adf_qclass_add, libadf), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, q2, lim, 128) == 0
            @test printed(y) == "union((0.5 +/- 0.51 ; 0 mod 3), (0.75 +/- 0.26 ; 1 mod 3)) + Q"
            ccall(ad(:adf_qclass_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), saved, y)
            # LIMIT before rounding and deduplication; the output stays untouched.
            @test ccall(ad(:adf_qclass_add), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, q2, 1, 128) == 10
            @test identical(y, saved)
            # Aliased output: z = y operand.
            ccall(ad(:adf_qclass_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), y, q2)
            @test ccall(ad(:adf_qclass_add), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, y, lim, 128) == 0
            @test identical(y, saved)
            lim = 1
            @test ccall((:adf_qclass_neg, libadf), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), y, q, lim, 128) == 0
            @test printed(y) == "union((0.25 +/- 0.26 ; 1 mod 3)) + Q"
            # Independent variation: x + x with z = x = y.
            readclass(q, "(0.25 +/- 0.25 ; 1 mod 3) + Q")
            @test ccall(ad(:adf_qclass_add), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), q, q, q, 1, 128) == 0
            @test printed(q) == "union((0.5 +/- 0.51 ; 2 mod 3)) + Q"
            # Count before deduplication, in place.
            readclass(q, "(1 +/- 1 ; 0 mod 1) + Q")
            ccall(ad(:adf_qclass_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), saved, q)
            @test ccall(ad(:adf_qclass_neg), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), q, q, 1, 128) == 10
            @test identical(q, saved)
            @test ccall(ad(:adf_qclass_neg), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong, Clong), q, q, 2, 128) == 0
            @test ccall(ad(:adf_qclass_length), Clong, (Ptr{Cvoid},), q) == 1
            @test printed(q) == "union((0.5 +/- 0.51 ; 0 mod 1)) + Q"
        finally
            for p in (q, q2, y, saved)
                ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), p)
            end
        end
    end
end
