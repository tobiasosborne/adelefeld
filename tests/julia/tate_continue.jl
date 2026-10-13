# Slice 5d (lane t5-slice4): docs/api-5.md 8 item 4, the same five arguments as tests/julia/tate.jl:
# ccall((:adf_tate_continue,lib),Cint,(P,P,P,L,L),z,chi,s,80,160) and the same for adf_tate_completed.
# Storage from the exported layout queries; an acb is the first member of cadele storage (as tests/julia/tate.jl).
# Expected values derived by hand before the run (lanes/t5-slice4/derive_fixture.py; conventions 9.5 at digits 5):
# zeta: Lambda(1/2) = -3.9769662255065128793, M = -3.977, R = ceil2(3.38e-5) = 3.4e-5, imaginary radius the tail
# and quadrature bound 2.0992e-26 (N = 4, R = 32); chi_4 = (4,3): I(1) = Lambda(1)/4 = 1/4, both radii
# 3.1807e-26/4 = 7.952e-27 (N = 8, R = 128); Lambda(-1, chi_4) = Lambda(2, chi_4) = 4 G/pi = 1.1662436161232751,
# M = 1.1662, R = ceil2(4.36e-5) = 4.4e-5, imaginary 3.3347e-26 (a trivial zero of L: Gamma(z) has a pole at z = 0).
using Test
using Libdl
length(ARGS) == 1 || error("usage: tate_continue.jl <libadelefeld.so>")
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
@testset "tate continuation and completed function" begin
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
            # zeta: Lambda(1/2), bits 80, prec 160
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, 1)
            ccall(fl(:acb_mul_2exp_si), Cvoid, (P, P, Clong), s, s, -1)
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), z, chi, s, 80, 160) == 0
            @test text(c, z, zero) == "((-3.977 +/- 3.4e-5) + (0 +/- 2.1e-26)*i ; 0)"
            # s = 0: the pole of zeta, DOMAIN (7) for both calls, z untouched
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, 0)
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), z, chi, s, 80, 160) == 7
            @test ccall(ad(:adf_tate_continue), Cint, (P, P, P, L, L), z, chi, s, 80, 160) == 7
            @test text(c, z, zero) == "((-3.977 +/- 3.4e-5) + (0 +/- 2.1e-26)*i ; 0)"
            # chi_4 at s = 1 (no pole for C > 1): I(1) = 1/4
            @test ccall(ad(:adf_char_set_conrey), Cint, (P, Culong, Culong), chi, 4, 3) == 0
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, 1)
            @test ccall(ad(:adf_tate_continue), Cint, (P, P, P, L, L), z, chi, s, 80, 160) == 0
            @test text(c, z, zero) == "((0.25 +/- 8e-27) + (0 +/- 8e-27)*i ; 0)"
            # chi_4 at its trivial zero s = -1: Lambda finite
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, -1)
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), z, chi, s, 80, 160) == 0
            @test text(c, z, zero) == "((1.1662 +/- 4.4e-5) + (0 +/- 3.4e-26)*i ; 0)"
            # bits -1: DOMAIN; prec above the cap: LIMIT (10)
            @test ccall(ad(:adf_tate_continue), Cint, (P, P, P, L, L), z, chi, s, -1, 160) == 7
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), z, chi, s, 80, 2097153) == 10
        finally
            ccall(ad(:adf_char_clear), Cvoid, (P,), chi)
            ccall(fl(:acb_clear), Cvoid, (P,), z)
            ccall(fl(:acb_clear), Cvoid, (P,), s)
            ccall(ad(:adf_cadele_clear), Cvoid, (P,), c)
            ccall(ad(:adf_fball_clear), Cvoid, (P,), zero)
        end
    end
end
