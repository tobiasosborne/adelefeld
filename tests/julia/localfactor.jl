# WP 1F.9 local zeta factor (lane f-slice14): adf_local_zeta_factor_at through ccall, the design's Julia call
# (docs/design/local-zeta.md, "Julia call") with the name of CV-60. Hand values: L_2(1) = 2; L_inf(2) = 1/pi;
# the exact pole 0 at p = 2 is DOMAIN (7) with y and where = 2 as the header says; a ball around -2 at the real
# place is NOT_DETERMINED (1); on OK where keeps its value.
# The acb layout is the documented target ABI (adele.h: acb_struct is 96 bytes, alignment 8).
using Test, Libdl
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB, n)
fl(n) = Libdl.dlsym(FLINT, n)
struct Place
    opaque::UInt64
end
const ACB = 96

function carrier(text)
    c = Ptr{UInt8}(Libc.malloc(ccall(ad(:adf_sizeof_cadele), Csize_t, ())))
    ccall(ad(:adf_cadele_init), Cvoid, (Ptr{UInt8},), c)
    @test ccall(ad(:adf_cadele_set_str), Cint, (Ptr{UInt8}, Cstring, Csize_t, Clong, Ptr{Cvoid}),
                c, text, sizeof(text), 128, C_NULL) == 0
    c
end
function release_carrier(c)
    ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{UInt8},), c)
    Libc.free(c)
end
function newacb()
    t = Ptr{UInt8}(Libc.malloc(ACB))
    ccall(fl(:acb_init), Cvoid, (Ptr{UInt8},), t)
    t
end
function freeacb(t)
    ccall(fl(:acb_clear), Cvoid, (Ptr{UInt8},), t)
    Libc.free(t)
end
zeta(y, w, s, v, prec) = ccall(ad(:adf_local_zeta_factor_at), Cint,
                               (Ptr{UInt8}, Ref{Place}, Ptr{UInt8}, Place, Clong), y, w, s, v, prec)

@testset "local zeta factor" begin
    s = newacb(); y = newacb(); saved = newacb()
    two = Ref{Clong}(0)
    ccall(fl(:fmpz_init), Cvoid, (Ref{Clong},), two)
    ccall(fl(:fmpz_set_si), Cvoid, (Ref{Clong}, Clong), two, 2)
    v = Ref(Place(0))
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, 2) == 0
    inf = ccall(ad(:adf_place_inf), Place, ())
    try
        c = carrier("((1) + (0)*i ; 0)")
        ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{UInt8}, Ptr{UInt8}), s, c)
        release_carrier(c)
        w = Ref(inf)
        st = zeta(y, w, s, v[], 128)
        println("status=", st)                       # expected: status=0
        @test st == 0
        @test ccall(fl(:acb_contains_fmpz), Cint, (Ptr{UInt8}, Ref{Clong}), y, two) == 1
        @test ccall(ad(:adf_place_is_archimedean), Cint, (Place,), w[]) == 1
        # the exact pole: DOMAIN, y untouched (the same representation), where = 2
        ccall(fl(:acb_set), Cvoid, (Ptr{UInt8}, Ptr{UInt8}), saved, y)
        ccall(fl(:acb_zero), Cvoid, (Ptr{UInt8},), s)
        st = zeta(y, w, s, v[], 128)
        println("status=", st)                       # expected: status=7
        @test st == 7
        @test ccall(ad(:adf_place_prime_get), Culong, (Place,), w[]) == 2
        @test ccall(fl(:acb_equal), Cint, (Ptr{UInt8}, Ptr{UInt8}), y, saved) == 1
        # the real place: L_inf(2) = 1/pi, computed in place (y = s)
        c = carrier("((2) + (0)*i ; 0)")
        ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{UInt8}, Ptr{UInt8}), s, c)
        release_carrier(c)
        st = zeta(s, w, s, inf, 128)
        @test st == 0
        ccall(fl(:acb_const_pi), Cvoid, (Ptr{UInt8}, Clong), saved, 400)
        ccall(fl(:acb_inv), Cvoid, (Ptr{UInt8}, Ptr{UInt8}, Clong), saved, saved, 400)
        @test ccall(fl(:acb_contains), Cint, (Ptr{UInt8}, Ptr{UInt8}), s, saved) == 1
        # a ball around the pole -2: NOT_DETERMINED, where = the real place
        c = carrier("((-2 +/- 0.001) + (0 +/- 0.001)*i ; 0)")
        ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{UInt8}, Ptr{UInt8}), s, c)
        release_carrier(c)
        w[] = v[]
        @test zeta(y, w, s, inf, 128) == 1
        @test ccall(ad(:adf_place_is_archimedean), Cint, (Place,), w[]) == 1
    finally
        ccall(fl(:fmpz_clear), Cvoid, (Ref{Clong},), two)
        freeacb(s); freeacb(y); freeacb(saved)
    end
end
