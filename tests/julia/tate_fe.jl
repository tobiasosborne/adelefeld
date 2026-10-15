# Slice 5e (lane t5-slice5): docs/api-5.md 8 item 5, the caller-computed functional equation: adf_tate_completed
# twice and adf_char_root_number once, with the ABI of tests/julia/tate_continue.jl:
# ccall((:adf_tate_completed,lib),Cint,(P,P,P,L,L),z,chi,s,bits,160), ccall((:adf_char_root_number,lib),Cint,
# (P,P,L),W,chi,wp). A = Lambda(s, chi) at bits 80; B = Lambda(1 - s, conj(chi)) at bits 84 (adf_char_conj);
# R = W B at wp = 160 = max(prec, bits + m + 16) for |B| < 2^64 (docs/api-5s.md "Slice 5e"). Checks: both OK,
# finite, the radius bound of acb_get_rad_ubound_arf at most 2^-81 (acb.rst:285-288), A and R overlapping, and the
# texts derived before the run (lanes/t5-slice5/derive_fixture.py; conventions 9.5 at digits 5): (5,2) at
# 3/8 + 3i/4: Lambda = 1.3464768750563704 + 0.37182350223562676 i, both sides print alike; chi_4 at its trivial zero
# -1: Lambda = 4G/pi = 1.1662436161232751, imaginary radii the bounds of the two calls (A: 3.3347e-26 at bits 80,
# B = Lambda(2, chi_4): 1.3152e-27 at bits 84; W = 1 exactly). zeta at s = 1: A is DOMAIN (7), and B at 0 too.
# Storage from the exported layout queries; an acb (and the arf of the radius bound) fits in cadele storage, whose
# first member is an acb (as tests/julia/tate.jl).
using Test
using Libdl
length(ARGS) == 1 || error("usage: tate_fe.jl <libadelefeld.so>")
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
narrow(z, u) = (ccall(fl(:acb_is_finite), Cint, (P,), z) != 0 &&
                (ccall(fl(:acb_get_rad_ubound_arf), Cvoid, (P, P, L), u, z, 64);
                 ccall(fl(:arf_cmp_2exp_si), Cint, (P, L), u, -81) <= 0))
@testset "tate functional equation" begin
    hb, chi = storage("char")
    kb, cc = storage("char")
    names = (:s, :t, :A, :B, :W, :R, :u)
    bufs = [storage("cadele") for _ in names]
    s, t, A, B, W, R, u = (b[2] for b in bufs)
    cb, c = storage("cadele")
    zb, zero = storage("fball")
    GC.@preserve hb kb bufs cb zb begin
        ccall(ad(:adf_char_init), Cvoid, (P,), chi)
        ccall(ad(:adf_char_init), Cvoid, (P,), cc)
        for z in (s, t, A, B, W, R)
            ccall(fl(:acb_init), Cvoid, (P,), z)
        end
        ccall(fl(:arf_init), Cvoid, (P,), u)
        ccall(ad(:adf_cadele_init), Cvoid, (P,), c)
        ccall(ad(:adf_fball_init), Cvoid, (P,), zero)
        try
            function sides(bits)
                ccall(ad(:adf_char_conj), Cvoid, (P, P), cc, chi)
                ccall(fl(:acb_neg), Cvoid, (P, P), t, s)
                ccall(fl(:acb_add_ui), Cvoid, (P, P, Culong, L), t, t, 1, 160)
                @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), A, chi, s, bits, 160) == 0
                @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), B, cc, t, bits + 4, 160) == 0
                @test ccall(ad(:adf_char_root_number), Cint, (P, P, L), W, chi, 160) == 0
                ccall(fl(:acb_mul), Cvoid, (P, P, P, L), R, W, B, 160)
                @test narrow(A, u) && narrow(R, u)
                @test ccall(fl(:acb_overlaps), Cint, (P, P), A, R) != 0
            end
            # (5,2), non-real and odd, at 3/8 + 3i/4
            @test ccall(ad(:adf_char_set_conrey), Cint, (P, Culong, Culong), chi, 5, 2) == 0
            ccall(fl(:acb_set_si_si), Cvoid, (P, Clong, Clong), s, 3, 6)
            ccall(fl(:acb_mul_2exp_si), Cvoid, (P, P, Clong), s, s, -3)
            sides(80)
            @test text(c, A, zero) == "((1.3465 +/- 2.4e-5) + (0.37182 +/- 3.6e-6)*i ; 0)"
            @test text(c, R, zero) == "((1.3465 +/- 2.4e-5) + (0.37182 +/- 3.6e-6)*i ; 0)"
            # chi_4 at its trivial zero s = -1
            @test ccall(ad(:adf_char_set_conrey), Cint, (P, Culong, Culong), chi, 4, 3) == 0
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, -1)
            sides(80)
            @test text(c, A, zero) == "((1.1662 +/- 4.4e-5) + (0 +/- 3.4e-26)*i ; 0)"
            @test text(c, R, zero) == "((1.1662 +/- 4.4e-5) + (0 +/- 1.4e-27)*i ; 0)"
            # zeta at s = 1: A is DOMAIN (7); B at 1 - s = 0 is DOMAIN as well; A untouched
            @test ccall(ad(:adf_char_set_conrey), Cint, (P, Culong, Culong), chi, 1, 1) == 0
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), s, 1)
            ccall(fl(:acb_set_si), Cvoid, (P, Clong), t, 0)
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), A, chi, s, 80, 160) == 7
            @test ccall(ad(:adf_tate_completed), Cint, (P, P, P, L, L), B, chi, t, 84, 160) == 7
            @test text(c, A, zero) == "((1.1662 +/- 4.4e-5) + (0 +/- 3.4e-26)*i ; 0)"
        finally
            ccall(ad(:adf_char_clear), Cvoid, (P,), chi)
            ccall(ad(:adf_char_clear), Cvoid, (P,), cc)
            for z in (s, t, A, B, W, R)
                ccall(fl(:acb_clear), Cvoid, (P,), z)
            end
            ccall(fl(:arf_clear), Cvoid, (P,), u)
            ccall(ad(:adf_cadele_clear), Cvoid, (P,), c)
            ccall(ad(:adf_fball_clear), Cvoid, (P,), zero)
        end
    end
end
