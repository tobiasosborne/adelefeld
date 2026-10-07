# Slice 5c (lane t5-slice2): docs/api-5.md 8 item 3,
# ccall((:adf_tate_integral,lib),Cint,(P,P,P,L,L),z,chi,s,64,128).
# Storage from the exported layout queries; an acb is the first member of cadele storage
# (as tests/julia/poisson.jl).
# Expected values derived by hand before the run (lanes/t5-slice2/derive_fixture.py; conventions 9.5 at digits 5):
# zeta: I(2) = pi/6 = 0.52359877559829887, M = 0.5236, R = ceil2(1.2244e-6) = 1.3e-6, imaginary radius the tail and
# quadrature bound 1.5937e-21 (N = 4, R = 16); chi_4 = (4,3): I(2) = G/(2 pi) = 0.14578045201540939, M = 0.14578,
# R = ceil2(4.5202e-7) = 4.6e-7, imaginary 1.3555e-21/8 = 1.6943e-22 (N = 8, R = 128).
using Test
using Libdl
length(ARGS) == 1 || error("usage: tate.jl <libadelefeld.so>")
const LIB = Libdl.dlopen(ARGS[1])
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
ad(name) = Libdl.dlsym(LIB, name)
fl(name) = Libdl.dlsym(FLINT, name)
const P = Ptr{Cvoid}
const L = Clong
function storage(kind)
    size = ccall(ad(Symbol("adf_sizeof_", kind)), Csize_t, ())
    align = ccall(ad(Symbol("adf_alignof_", kind)), Csize_t, ())
    bytes = Vector{UInt8}(undef, size + align - 1)
    p = P((UInt(pointer(bytes)) + align - 1) & ~(UInt(align) - 1))
    return bytes, p
end
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
@testset "tate integral" begin
    hb, chi = storage("char")
    zb, z = storage("cadele")
    sb, s = storage("cadele")
    cb, c = storage("cadele")
    bb, zero = storage("fball")
    GC.@preserve hb zb sb cb bb begin
        ccall(ad(:adf_char_init), Cvoid, (P,), chi)
        ccall(fl(:acb_init), Cvoid, (P,), z)
        ccall(fl(:acb_init), Cvoid, (P,), s)
        ccall(ad(:adf_cadele_init), Cvoid, (P,), c)
        ccall(ad(:adf_fball_init), Cvoid, (P,), zero)
        try
            ccall(fl(:acb_set_ui), Cvoid, (P, Culong), s, 2)
            # the design's call: zeta at s = 2, bits 64, prec 128
            @test ccall(ad(:adf_tate_integral), Cint, (P, P, P, L, L), z, chi, s, 64, 128) == 0
            @test text(c, z, zero) == "((0.5236 +/- 1.3e-6) + (0 +/- 1.6e-21)*i ; 0)"
            # chi_4 at s = 2
            @test ccall(ad(:adf_char_set_conrey), Cint, (P, Culong, Culong), chi, 4, 3) == 0
            @test ccall(ad(:adf_tate_integral), Cint, (P, P, P, L, L), z, chi, s, 64, 128) == 0
            @test text(c, z, zero) == "((0.14578 +/- 4.6e-7) + (0 +/- 1.7e-22)*i ; 0)"
            # s = 1: DOMAIN (7), z untouched; bits -1: DOMAIN; prec above the cap: LIMIT (10)
            ccall(fl(:acb_set_ui), Cvoid, (P, Culong), s, 1)
            @test ccall(ad(:adf_tate_integral), Cint, (P, P, P, L, L), z, chi, s, 64, 128) == 7
            @test text(c, z, zero) == "((0.14578 +/- 4.6e-7) + (0 +/- 1.7e-22)*i ; 0)"
            ccall(fl(:acb_set_ui), Cvoid, (P, Culong), s, 2)
            @test ccall(ad(:adf_tate_integral), Cint, (P, P, P, L, L), z, chi, s, -1, 128) == 7
            @test ccall(ad(:adf_tate_integral), Cint, (P, P, P, L, L), z, chi, s, 64, 2097153) == 10
        finally
            ccall(ad(:adf_char_clear), Cvoid, (P,), chi)
            ccall(fl(:acb_clear), Cvoid, (P,), z)
            ccall(fl(:acb_clear), Cvoid, (P,), s)
            ccall(ad(:adf_cadele_clear), Cvoid, (P,), c)
            ccall(ad(:adf_fball_clear), Cvoid, (P,), zero)
        end
    end
end
