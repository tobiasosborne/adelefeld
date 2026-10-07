# Slice 4a: all storage comes from layout queries. F4 swaps (4,1) to (1,4).
using Test, Libdl
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB, n)
const P = Ptr{Cvoid}
function storage()
    size = ccall(ad(:adf_sizeof_ffun), Csize_t, ())
    align = ccall(ad(:adf_alignof_ffun), Csize_t, ())
    @test size > 0 && align > 0 && ispow2(align)
    bytes = Vector{UInt8}(undef, size + align - 1)
    ptr = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    @test UInt(ptr) % align == 0
    bytes, ptr
end
function value(x)
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_ffun_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, x, 20)
    @test s != C_NULL
    try
        unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (Ptr{UInt8},), s)
    end
end
@testset "slice 4a" begin
    fb, f = storage(); gb, g = storage(); hb, h = storage()
    text = "ffun(D=4, M=1; (0) + (0)*i, (1) + (0)*i, (0) + (0)*i, (0) + (0)*i)"
    expected = "ffun(D=1, M=4; (1) + (0)*i, (0) + (-1)*i, (-1) + (0)*i, (0) + (1)*i)"
    reflected = "ffun(D=4, M=1; (0) + (0)*i, (0) + (0)*i, (0) + (0)*i, (1) + (0)*i)"
    GC.@preserve fb gb hb text expected reflected begin
        ccall(ad(:adf_ffun_init), Cvoid, (P,), f)
        ccall(ad(:adf_ffun_init), Cvoid, (P,), g)
        ccall(ad(:adf_ffun_init), Cvoid, (P,), h)
        try
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, text, sizeof(text), 128, C_NULL) == 0
            @test ccall(ad(:adf_ffun_is_canonical), Cint, (P,), f) == 1
            @test ccall(ad(:adf_ffun_fourier), Cint, (P, P, Clong), g, f, 128) == 0
            @test value(g) == expected
            @test ccall(ad(:adf_ffun_fourier), Cint, (P, P, Clong), h, g, 128) == 0
            @test value(h) == reflected
            ccall(ad(:adf_ffun_set), Cvoid, (P, P), h, g)
            @test ccall(ad(:adf_ffun_fourier), Cint, (P, P, Clong), g, f, 2097153) == 10
            @test ccall(ad(:adf_ffun_identical), Cint, (P, P), g, h) == 1
            @test ccall(ad(:adf_ffun_fourier), Cint, (P, P, Clong), f, f, 128) == 0
            @test ccall(ad(:adf_ffun_identical), Cint, (P, P), f, g) == 1
            @test ccall(ad(:adf_ffun_add), Cint, (P, P, P, Clong), h, f, g, 128) == 0
            @test ccall(ad(:adf_ffun_is_canonical), Cint, (P,), h) == 1
        finally
            ccall(ad(:adf_ffun_clear), Cvoid, (P,), h)
            ccall(ad(:adf_ffun_clear), Cvoid, (P,), g)
            ccall(ad(:adf_ffun_clear), Cvoid, (P,), f)
        end
    end
end
