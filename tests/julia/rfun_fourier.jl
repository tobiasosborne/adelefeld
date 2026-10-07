# Slice 4e (lane f4-slice3): docs/api-4.md 9 item 5, ccall((:adf_rfun_fourier,lib),Cint,(P,P,Clong),out,r,128).
# Storage from the exported layout queries; every string freed with adf_str_free.
using Test
using Libdl
length(ARGS) == 1 || error("usage: rfun_fourier.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
ad(name) = Libdl.dlsym(LIB, name)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
function text(f)
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_rfun_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, f, 20)
    s == C_NULL && return nothing
    try
        return unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (P,), s)
    end
end
@testset "rfun fourier" begin
    rb, r = storage("rfun")
    ob, out = storage("rfun")
    odd = "rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    # F(x exp(-pi x^2)) = i y exp(-pi y^2), exactly (api-4.md R1 step 2)
    hat = "rfun(term(P=[(0) + (0)*i, (0) + (1)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    # F^2 is the reflection: -x exp(-pi x^2)
    twice = "rfun(term(P=[(0) + (0)*i, (-1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    GC.@preserve rb ob odd begin
        ccall(ad(:adf_rfun_init), Cvoid, (P,), r)
        ccall(ad(:adf_rfun_init), Cvoid, (P,), out)
        try
            @test ccall(ad(:adf_rfun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        r, odd, sizeof(odd), 128, C_NULL) == 0
            @test ccall(ad(:adf_rfun_fourier), Cint, (P, P, Clong), out, r, 128) == 0
            @test ccall(ad(:adf_rfun_is_canonical), Cint, (P,), out) == 1
            @test text(out) == hat
            @test ccall(ad(:adf_rfun_fourier), Cint, (P, P, Clong), out, out, 128) == 0
            @test text(out) == twice
            # prec above ADF_REAL_PREC_MAX: LIMIT (10), out untouched
            @test ccall(ad(:adf_rfun_fourier), Cint, (P, P, Clong), out, r, 2097153) == 10
            @test text(out) == twice
            # the derivative of x exp(-pi x^2) is (1 - 2 pi x^2) exp(-pi x^2): three coefficients
            @test ccall(ad(:adf_rfun_derivative), Cint, (P, P, Clong), out, r, 128) == 0
            @test ccall(ad(:adf_rfun_is_canonical), Cint, (P,), out) == 1
            @test startswith(text(out), "rfun(term(P=[(1) + (0)*i, (0) + (0)*i, (-6.2831853071795864769")
        finally
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), r)
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), out)
        end
    end
end
