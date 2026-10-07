# Slice 5a, api-5 section 8. Constructors supply Place by value.
# Raw acb allocation uses the documented FLINT 3.0.1 target ABI (96 bytes).
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
            (Ptr{Cvoid}, Cstring, Csize_t, Clong, Ptr{Cvoid}), c, text, sizeof(text), 128, C_NULL) == 0
        ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), p, c)
    finally
        ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{Cvoid},), c)
        Libc.free(c)
    end
end
tate(z, where, s, v, alpha, eta0, prec=128) = ccall(ad(:adf_local_tate_at), Cint,
    (Ptr{Cvoid}, Ref{Place}, Ptr{Cvoid}, Place, Ptr{Cvoid}, Ptr{Cvoid}, Clong),
    z, where, s, v, alpha, eta0, prec)

@testset "local Tate integral" begin
    s = newacb(); alpha = newacb(); z = newacb(); ref = newacb(); saved = newacb()
    chi = Ptr{Cvoid}(Libc.malloc(ccall(ad(:adf_sizeof_char), Csize_t, ())))
    ccall(ad(:adf_char_init), Cvoid, (Ptr{Cvoid},), chi)
    v = Ref(Place(0))
    @test ccall(ad(:adf_place_prime), Cint, (Ref{Place}, Culong), v, 2) == 0
    inf = ccall(ad(:adf_place_inf), Place, ())
    where = Ref(inf)
    try
        settext(s, "((2) + (0)*i ; 0)")
        ccall(fl(:acb_one), Cvoid, (Ptr{Cvoid},), alpha)
        @test tate(z, where, s, v[], alpha, chi) == 0
        ccall(fl(:acb_set_ui), Cvoid, (Ptr{Cvoid}, Culong), ref, 4)
        ccall(fl(:acb_div_ui), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Culong, Clong), ref, ref, 3, 400)
        @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, ref) == 1
        @test where[] == inf
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 4, 3) == 0
        settext(alpha, "((0) + (1)*i ; 0)")
        @test tate(alpha, where, s, v[], alpha, chi) == 0
        @test ccall(fl(:acb_is_one), Cint, (Ptr{Cvoid},), alpha) == 1
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 1, 1) == 0
        @test tate(s, where, s, inf, alpha, chi) == 0
        ccall(fl(:acb_const_pi), Cvoid, (Ptr{Cvoid}, Clong), ref, 400)
        ccall(fl(:acb_inv), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}, Clong), ref, ref, 400)
        @test ccall(fl(:acb_contains), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), s, ref) == 1
        ccall(fl(:acb_set), Cvoid, (Ptr{Cvoid}, Ptr{Cvoid}), saved, z)
        ccall(fl(:acb_zero), Cvoid, (Ptr{Cvoid},), s)
        @test tate(z, where, s, v[], alpha, chi) == 7
        @test where[] == v[]
        @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, saved) == 1
        settext(s, "((0 +/- 0.001) + (0)*i ; 0)")
        @test tate(z, where, s, v[], alpha, chi) == 1
        @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, saved) == 1
        @test ccall(ad(:adf_char_set_conrey), Cint, (Ptr{Cvoid}, Culong, Culong), chi, 4, 3) == 0
        settext(s, "((-1) + (0)*i ; 0)")
        @test tate(z, where, s, inf, alpha, chi) == 7
        @test where[] == inf
        @test ccall(fl(:acb_equal), Cint, (Ptr{Cvoid}, Ptr{Cvoid}), z, saved) == 1
    finally
        for p in (s, alpha, z, ref, saved)
            freeacb(p)
        end
        ccall(ad(:adf_char_clear), Cvoid, (Ptr{Cvoid},), chi)
        Libc.free(chi)
    end
end
