# Slice 3.1-e. D3-1 status plus truth; Q2 exact set queries, including the glued pair.
using Test
using Libdl

length(ARGS) == 1 || error("usage: qclass_sets.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
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

@testset "qclass exact sets" begin
    xb, x = storage("qclass")
    yb, y = storage("qclass")
    truth = Ref{Cint}(719)
    GC.@preserve xb yb truth begin
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), y)
        try
            readclass(x, "(1 ; 1) + Q")
            readclass(y, "(0 ; 0) + Q")
            @test ccall(ad(:adf_qclass_equal_set), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 6) == 0
            @test truth[] == 1
            truth[] = 719
            @test ccall(ad(:adf_qclass_equal_set), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 5) == 10
            @test truth[] == 719
            readclass(x, "(0.75 +/- 0.25 ; 0 mod 3) + Q")
            readclass(y, "(0.25 +/- 0.25 ; 2 mod 3) + Q")
            @test ccall(ad(:adf_qclass_equal_set), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 42) == 0
            @test truth[] == 0
            @test ccall(ad(:adf_qclass_contains), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 42) == 0
            @test truth[] == 0
            @test ccall(ad(:adf_qclass_overlaps), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 42) == 0
            @test truth[] == 1
            for fn in (:adf_qclass_equal_set, :adf_qclass_contains, :adf_qclass_overlaps)
                @test ccall(ad(fn), Cint,
                    (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, x, 1000) == 0
                @test truth[] == 1
                truth[] = 719
                @test ccall(ad(fn), Cint,
                    (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, x, 0) == 10
                @test truth[] == 719
            end
            readclass(x, "(0.5 ; 0) + Q")
            readclass(y, "(0.5 +/- 0.25 ; 0 mod 3) + Q")
            @test ccall(ad(:adf_qclass_contains), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, x, y, 1000) == 0
            @test truth[] == 1
            @test ccall(ad(:adf_qclass_contains), Cint,
                (Ref{Cint}, Ptr{Cvoid}, Ptr{Cvoid}, Clong), truth, y, x, 1000) == 0
            @test truth[] == 0
        finally
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), y)
        end
    end
end
