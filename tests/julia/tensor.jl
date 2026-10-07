# Slice 4f (lane f4-slice5): docs/api-4.md 9 item 6, ccall((:adf_tensor_eval,lib),Cint,(P,P,P,P,Clong),z,r,f,x,128).
# Storage from the exported layout queries; an acb is the first member of cadele storage (as tests/julia/psi.jl).
using Test
using Libdl
length(ARGS) == 1 || error("usage: tensor.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
ad(name) = Libdl.dlsym(LIB, name)
fl(name) = Libdl.dlsym(FLINT, name)
const P = Ptr{Cvoid}
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
# z printed as the real and imaginary parts of a complex adele with the finite part 0, digits 5
function text(c, z, zero)
    @assert ccall(ad(:adf_cadele_set_acb_fball), Cint, (P, P, P), c, z, zero) == 0
    n = Ref{Csize_t}(0)
    s = ccall(ad(:adf_cadele_get_str), Ptr{UInt8}, (Ref{Csize_t}, P, Clong), n, c, 5)
    try
        return unsafe_string(s, n[])
    finally
        ccall(ad(:adf_str_free), Cvoid, (P,), s)
    end
end
@testset "tensor eval" begin
    rb, r = storage("rfun")
    fb, f = storage("ffun")
    xb, x = storage("adele")
    zb, z = storage("cadele")
    cb, c = storage("cadele")
    bb, zero = storage("fball")
    gauss = "rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))"
    six = "ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)"
    GC.@preserve rb fb xb zb cb bb gauss six begin
        ccall(ad(:adf_rfun_init), Cvoid, (P,), r)
        ccall(ad(:adf_ffun_init), Cvoid, (P,), f)
        ccall(ad(:adf_adele_init), Cvoid, (P,), x)
        ccall(fl(:acb_init), Cvoid, (P,), z)
        ccall(ad(:adf_cadele_init), Cvoid, (P,), c)
        ccall(ad(:adf_fball_init), Cvoid, (P,), zero)
        try
            @test ccall(ad(:adf_rfun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        r, gauss, sizeof(gauss), 128, C_NULL) == 0
            @test ccall(ad(:adf_ffun_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        f, six, sizeof(six), 128, C_NULL) == 0
            # (0.25 ; 7): exp(-pi/16) f(7) = 3 exp(-pi/16) = 2.4651748741 (7 = 14/2, index 2, value 3)
            pt = "(0.25 ; 7)"
            @test ccall(ad(:adf_adele_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        x, pt, sizeof(pt), 128, C_NULL) == 0
            @test ccall(ad(:adf_tensor_eval), Cint, (P, P, P, P, Clong), z, r, f, x, 128) == 0
            @test text(c, z, zero) == "((2.4652 +/- 2.6e-5) + (0)*i ; 0)"
            # (0 ; 1/4 mod 1): (1/4) + Zhat meets no coset of (1/2) Zhat: exactly 0 (E1 step 2)
            pt = "(0 ; 1/4 mod 1)"
            @test ccall(ad(:adf_adele_set_str), Cint, (P, Ptr{UInt8}, Csize_t, Clong, P),
                        x, pt, sizeof(pt), 128, C_NULL) == 0
            @test ccall(ad(:adf_tensor_eval), Cint, (P, P, P, P, Clong), z, r, f, x, 128) == 0
            @test ccall(fl(:acb_is_zero), Cint, (P,), z) == 1
            # prec above ADF_REAL_PREC_MAX: LIMIT (10), z untouched
            @test ccall(ad(:adf_tensor_eval), Cint, (P, P, P, P, Clong), z, r, f, x, 2097153) == 10
            @test ccall(fl(:acb_is_zero), Cint, (P,), z) == 1
        finally
            ccall(ad(:adf_rfun_clear), Cvoid, (P,), r)
            ccall(ad(:adf_ffun_clear), Cvoid, (P,), f)
            ccall(ad(:adf_adele_clear), Cvoid, (P,), x)
            ccall(fl(:acb_clear), Cvoid, (P,), z)
            ccall(ad(:adf_cadele_clear), Cvoid, (P,), c)
            ccall(ad(:adf_fball_clear), Cvoid, (P,), zero)
        end
    end
end
