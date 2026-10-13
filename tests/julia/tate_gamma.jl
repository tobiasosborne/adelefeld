# Slice 5b, api-5 section 8 item 2: adf_local_gamma_at and adf_local_epsilon_at with the ABI of
# tests/julia/tate_local.jl (Place by value from the constructors). Raw acb allocation uses the documented
# FLINT 3.0.1 target ABI (96 bytes). References: -3/4 (P9 a=0 at p=2, alpha=1, s=2, by hand);
# G_minus for eta0=(5,2) at s=0, alpha=1: -2 sin(4pi/5) - 2i sin(2pi/5) (60 digits, mpmath); 1 and i at s=1/2.
using Test, Libdl
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB, n)
fl(n) = Libdl.dlsym(FLINT, n)
struct Place
    opaque::Culong
end
function newacb()
    p = Ptr{Cvoid}(Libc.malloc(96))
    ccall(fl(:acb_init), Cvoid, (Ptr{Cvoid},), p)
    p
end
function freeacb(p)
    ccall(fl(:acb_clear), Cvoid, (Ptr{Cvoid},), p)
    Libc.free(p)
end
function settext(p, text)
    c = Ptr{Cvoid}(Libc.malloc(ccall(ad(:adf_sizeof_cadele), Csize_t, ())))
    ccall(ad(:adf_cadele_init), Cvoid, (Ptr{Cvoid},), c)
    try
        @test ccall(ad(:adf_cadele_set_str), Cint,
            (Ptr{Cvoid}, Cstring, Csize_t, Clong, Ptr{Cvoid}), c, text, sizeof(text), 400, C_NULL) == 0
        ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), p, c)
    finally
        ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{Cvoid},), c)
        Libc.free(c)
    end
end
local_call(sym, z, where, s, v, alpha, eta0, prec=128) = ccall(ad(sym), Cint,
    (Ptr{Cvoid}, Ref{Place}, Ptr{Cvoid}, Place, Ptr{Cvoid}, Ptr{Cvoid}, Clong),
    z, where, s, v, alpha, eta0, prec)
contains(z, ref) = ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, ref) == 1
equal(x, y) = ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), x, y) == 1

@testset "local gamma and epsilon" begin
    s = newacb(); alpha = newacb(); z = newacb(); ref = newacb(); saved = newacb()
    chi = Ptr{Cvoid}(Libc.malloc(ccall(ad(:adf_sizeof_char), Csize_t, ())))
    ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), chi)
    two = Ref(Place(0)); three = Ref(Place(0)); five = Ref(Place(0))
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), two, 2) == 0
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), three, 3) == 0
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), five, 5) == 0
    inf = ccall(ad(:adf_place_inf), Place, ())
    where = Ref(inf)
    try
        # unramified quotient -3/4, contained exactly
        settext(s, "((2) + (0)*i ; 0)")
        ccall(fl(:acb_one), Cvoid, (Ptr{Cvoid},), alpha)
        @test local_call(:adf_local_gamma_at, z, where, s, two[], alpha, chi) == 0
        settext(ref, "((-0.75) + (0)*i ; 0)")
        @test contains(z, ref)
        @test where[] == inf
        # ramified: eta0 = (5,2), alpha = 1, s = 0, the negative sum G_minus(eta0^-1)
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 5, 2) == 0
        settext(s, "((0) + (0)*i ; 0)")
        @test local_call(:adf_local_gamma_at, z, where, s, five[], alpha, chi) == 0
        settext(ref, "((-1.17557050458494625833741190927814553719530487528629198214454) + " *
                     "(-1.90211303259030714423287866675876428681139726825150044489461)*i ; 0)")
        @test contains(z, ref)
        @test local_call(:adf_local_epsilon_at, saved, where, s, five[], alpha, chi) == 0
        @test equal(saved, z)
        # real place: e = 0 and e = 1 at s = 1/2; alias z = s
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 1, 1) == 0
        settext(s, "((0.5) + (0)*i ; 0)")
        @test local_call(:adf_local_gamma_at, s, where, s, inf, alpha, chi) == 0
        ccall(fl(:acb_one), Cvoid, (Ptr{Cvoid},), ref)
        @test contains(s, ref)
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 4, 3) == 0
        settext(s, "((0.5) + (0)*i ; 0)")
        @test local_call(:adf_local_gamma_at, z, where, s, inf, alpha, chi) == 0
        ccall(fl(:acb_onei), Cvoid, (Ptr{Cvoid},), ref)
        @test contains(z, ref)
        # epsilon at a gamma pole (e = 1, s = 2) is exactly i; gamma there is DOMAIN, atomic, where = v
        settext(s, "((2) + (0)*i ; 0)")
        @test local_call(:adf_local_epsilon_at, z, where, s, inf, alpha, chi) == 0
        @test equal(z, ref)
        ccall(fl(:acb_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), saved, z)
        where[] = two[]
        @test local_call(:adf_local_gamma_at, z, where, s, inf, alpha, chi) == 7
        @test where[] == inf
        @test equal(z, saved)
        # a ball meeting the pole s = 1 at p = 3, alpha = 1: NOT_DETERMINED, z untouched
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 1, 1) == 0
        settext(s, "((1 +/- 0.001) + (0)*i ; 0)")
        @test local_call(:adf_local_gamma_at, z, where, s, three[], alpha, chi) == 1
        @test where[] == three[]
        @test equal(z, saved)
        @test local_call(:adf_local_epsilon_at, z, where, s, three[], alpha, chi) == 0
        ccall(fl(:acb_one), Cvoid, (Ptr{Cvoid},), ref)
        @test equal(z, ref)
    finally
        for p in (s, alpha, z, ref, saved)
            freeacb(p)
        end
        ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), chi)
        Libc.free(chi)
    end
end
