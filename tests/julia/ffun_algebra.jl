# Slice 4b: the design section 9 dilation ccall and all finite algebra symbols.
using Test, Libdl
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB, n)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    @test size > 0 && ispow2(align)
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    @test UInt(p) % align == 0
    bytes, p
end
function value(f)
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_ffun_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, f, 20)
    @test s != C_NULL
    try
        unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (P,), s)
    end
end
function rational(q, text)
    GC.@preserve text begin
        @test ccall(ad(:adf_rat_set_str), Cint, (P, Ptr{UInt8}, Csize_t, P),
                    q, text, sizeof(text), C_NULL) == 0
    end
end
@testset "slice 4b finite algebra" begin
    fb, f = storage("ffun"); gb, g = storage("ffun"); hb, h = storage("ffun")
    qb, q = storage("rat"); ab, a = storage("idele")
    delta = "ffun(D=2, M=1; (0) + (0)*i, (1) + (0)*i)"
    translated = "ffun(D=2, M=1; (1) + (0)*i, (0) + (0)*i)"
    cells = ["(0) + (0)*i" for j in 0:11]
    cells[4] = cells[10] = "(1) + (0)*i"
    expected = "ffun(D=4, M=3; " * join(cells, ", ") * ")"
    ambiguous = "(7 ; 1 * [1 mod 1])"
    finite = "ffun(D=1, M=3; (0) + (0)*i, (1) + (0)*i, (0) + (0)*i)"
    GC.@preserve fb gb hb qb ab delta translated expected ambiguous finite begin
        for p in [f, g, h]
            ccall(ad(:adf_ffun_init), Cvoid, (P,), p)
        end
        ccall(ad(:adf_rat_init), Cvoid, (P,), q)
        ccall(ad(:adf_idele_init), Cvoid, (P,), a)
        try
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, delta, sizeof(delta), 128, C_NULL) == 0
            rational(q, "2/3")
            @test ccall(ad(:adf_ffun_dilate_rat), Cint, (P, P, P), g, f, q) == 0
            @test value(g) == expected
            @test ccall(ad(:adf_ffun_dilate_rat), Cint, (P, P, P), f, f, q) == 0
            @test ccall(ad(:adf_ffun_identical), Cint, (P, P), f, g) == 1
            ccall(ad(:adf_ffun_set), Cvoid, (P, P), h, g)
            rational(q, "0")
            @test ccall(ad(:adf_ffun_dilate_rat), Cint, (P, P, P), g, f, q) == 7
            @test ccall(ad(:adf_ffun_identical), Cint, (P, P), g, h) == 1
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, delta, sizeof(delta), 128, C_NULL) == 0
            rational(q, "1/2")
            @test ccall(ad(:adf_ffun_translate_rat), Cint, (P, P, P), g, f, q) == 0
            @test value(g) == translated
            @test ccall(ad(:adf_ffun_reflect), Cint, (P, P), h, f) == 0
            @test value(h) == delta
            @test ccall(ad(:adf_ffun_conj), Cint, (P, P), h, f) == 0
            @test value(h) == delta
            @test ccall(ad(:adf_ffun_mul), Cint, (P, P, P, Clong), h, f, f, 128) == 0
            @test value(h) == delta
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, finite, sizeof(finite), 128, C_NULL) == 0
            @test ccall(ad(:adf_idele_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        a, ambiguous, sizeof(ambiguous), 128, C_NULL) == 0
            ccall(ad(:adf_ffun_set), Cvoid, (P, P), h, g)
            @test ccall(ad(:adf_ffun_dilate_idele), Cint, (P, P, P), g, f, a) == 1
            @test ccall(ad(:adf_ffun_identical), Cint, (P, P), g, h) == 1
        finally
            ccall(ad(:adf_idele_clear), Cvoid, (P,), a)
            ccall(ad(:adf_rat_clear), Cvoid, (P,), q)
            for p in [f, g, h]
                ccall(ad(:adf_ffun_clear), Cvoid, (P,), p)
            end
        end
    end
end
