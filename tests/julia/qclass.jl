# Slice 3.1-a. Allocate by exported size and alignment, and preserve byte storage and strings.
# Only the declared lift operations are called. All owned values and returned strings are released.
using Test
using Libdl

length(ARGS) == 1 || error("usage: qclass.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
ad(name) = Libdl.dlsym(LIB, name)

function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = Ptr{Cvoid}((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end

@testset "qclass lift and rational translation" begin
    xb, x = storage("adele")
    ab, a = storage("adele")
    qb, q = storage("qclass")
    yb, y = storage("qclass")
    rb, r = storage("rat")
    text, rat = "(0 ; 1/3)", "-7/3"
    GC.@preserve xb ab qb yb rb text rat begin
        ccall(ad(:adf_adele_init), Cvoid, (Ptr{Cvoid},), x)
        ccall(ad(:adf_adele_init), Cvoid, (Ptr{Cvoid},), a)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), q)
        ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), y)
        ccall(ad(:adf_rat_init), Cvoid, (Ptr{Cvoid},), r)
        try
            @test ccall(ad(:adf_sizeof_qclass), Csize_t, ()) == 24
            @test ccall(ad(:adf_alignof_qclass), Csize_t, ()) == 8
            @test ccall(ad(:adf_adele_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), x, text, sizeof(text), 128, C_NULL) == 0
            @test ccall(ad(:adf_rat_set_str), Cint,
                (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Ptr{Cvoid}), r, rat, sizeof(rat), C_NULL) == 0
            ccall(ad(:adf_qclass_set_adele), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, x)
            @test ccall(ad(:adf_qclass_is_canonical), Cint, (Ptr{Cvoid},), q) == 1
            @test ccall(ad(:adf_qclass_form), Cint, (Ptr{Cvoid},), q) == 0
            @test ccall(ad(:adf_qclass_length), Clong, (Ptr{Cvoid},), q) == 1
            @test ccall(ad(:adf_qclass_get_piece), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), a, q, 0) == 0
            @test ccall(ad(:adf_adele_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), a, x) == 1
            ccall(ad(:adf_qclass_add_rat), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), y, q, r)
            @test ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), q, y) == 1
            ccall(ad(:adf_qclass_add_rat), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Ptr{Cvoid}), q, q, r)
            @test ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), q, y) == 1
            n = Ref{Csize_t}(0)
            s = ccall(ad(:adf_qclass_get_str), Ptr{UInt8}, (Ref{Csize_t}, Ptr{Cvoid}, Clong), n, y, 20)
            @test s != C_NULL
            if s != C_NULL
                try
                    @test unsafe_string(s, n[]) == "(0 ; 1/3) + Q"
                    @test ccall(ad(:adf_qclass_set_str), Cint,
                        (Ptr{Cvoid}, Ptr{UInt8}, Csize_t, Clong, Ptr{Cvoid}), q, s, n[], 128, C_NULL) == 0
                    @test ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), q, y) == 1
                finally
                    ccall(ad(:adf_str_free), Cvoid, (Ptr{Cvoid},), s)
                end
            end
            ccall(ad(:adf_qclass_set_rat), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, r)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), y)
            ccall(ad(:adf_qclass_init), Cvoid, (Ptr{Cvoid},), y)
            @test ccall(ad(:adf_qclass_identical), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), q, y) == 1
            ccall(ad(:adf_qclass_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, q)
            ccall(ad(:adf_qclass_swap), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), q, q)
            @test ccall(ad(:adf_qclass_get_piece), Cint,
                (Ptr{Cvoid}, Ptr{Cvoid}, Clong), a, q, 1) == 7
        finally
            ccall(ad(:adf_adele_clear), Cvoid, (Ptr{Cvoid},), x)
            ccall(ad(:adf_adele_clear), Cvoid, (Ptr{Cvoid},), a)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), q)
            ccall(ad(:adf_qclass_clear), Cvoid, (Ptr{Cvoid},), y)
            ccall(ad(:adf_rat_clear), Cvoid, (Ptr{Cvoid},), r)
        end
    end
end
